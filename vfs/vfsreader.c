
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "vfsreader.h"

//#define __HAVE_PTHREAD__

#ifdef __USE_ZLIB__
#include <zlib.h>
#endif

#ifdef _WIN32
#error("not implement yet")
#else

#include <pthread.h>
typedef pthread_mutex_t mutex_t;
#define mutext_init(a, b) pthread_mutex_init(a, b)
#define mutext_lock(a) pthread_mutex_lock(a)
#define mutext_unlock(a) pthread_mutex_unlock(a)
#define mutext_destory(a) pthread_mutex_destroy
#endif

#define ANY_SIZE 1

#define MAX_PATH (512)

#define GET_PARENT_ID(e) (e->id & 0x7FFFFFFF)
#define IS_DIR(e) (e->id & 0x80000000)

typedef struct _vfs_dir_entry
{
    unsigned int id;
    unsigned int file_size;
#ifdef __USE_ZLIB__
    unsigned int compress_size;
#endif
    long offset;
    char name[ANY_SIZE];
} vfs_dir_entry_t;

#ifdef __USE_ZLIB__
#define NODE_SIZE (13)
#else
#define NODE_SIZE (9)
#endif

int vfs_file_size(vfs_dir_entry_p entry)
{
    return entry->file_size;
}

const char *vfs_file_name(vfs_dir_entry_p entry)
{
    return entry->name;
}

typedef struct _vfs_dir_entry_array
{
    int size;
    int allocate_size;
    vfs_dir_entry_p *data;
} vfs_dir_entry_array_t, *vfs_dir_entry_array_p;

struct _vfs_handle
{
    vfs_dir_entry_array_t dirs;
    vfs_dir_entry_array_t files;
    FILE *host_file;
    mutex_t lock;
#ifdef __USE_ZLIB__
    void *dic;
    int dic_size;
#endif
};

typedef struct _vfs_handle vfs_handle_t;

int init_vfs_dir_entry_array(vfs_dir_entry_array_p array, int allocate_size)
{
    array->size = 0;
    array->allocate_size = allocate_size;
    array->data = (vfs_dir_entry_p *)malloc(sizeof(vfs_dir_entry_p) * array->allocate_size);
    if (array->data == NULL)
    {
        return ERROR_NO_MEMORY;
    }
    memset(array->data, 0, sizeof(vfs_dir_entry_p) * array->allocate_size);
    return ERROR_SUCCESS;
}

void free_vfs_dir_entry_array(vfs_dir_entry_array_p array)
{
    for (int i = 0; i < array->size; i++)
    {
        free(array->data[i]);
    }
    free(array->data);
}

int extend_vfs_dir_entry_array(vfs_dir_entry_array_p array, int add_size)
{
    int new_size = 0;
    if (array->allocate_size - array->size > add_size)
    {
        return ERROR_SUCCESS;
    }
    new_size = (add_size / 1024 + 1) * 1024 + array->allocate_size;
    vfs_dir_entry_p *new_data = (vfs_dir_entry_p *)malloc(new_size * sizeof(vfs_dir_entry_p));
    if (new_data == NULL)
    {
        return ERROR_NO_MEMORY;
    }
    memset(new_data, 0, new_size * sizeof(vfs_dir_entry_p));
    memcpy(new_data, array->data, array->allocate_size * sizeof(vfs_dir_entry_p));
    array->allocate_size = new_size;
    free(array->data);
    array->data = new_data;
    return ERROR_SUCCESS;
}

int add_vfs_dir_entry(vfs_dir_entry_array_p array, vfs_dir_entry_p entry)
{
    int error = extend_vfs_dir_entry_array(array, 1);
    if (error != ERROR_SUCCESS)
    {
        return error;
    }
    array->data[array->size] = entry;
    array->size++;
    return ERROR_SUCCESS;
}

int move_dir_entry(vfs_dir_entry_array_p to_array, vfs_dir_entry_array_p from_array)
{
    int error = extend_vfs_dir_entry_array(to_array, from_array->size);
    if (error != 0)
    {
        return error;
    }
    memcpy(&to_array->data[to_array->size], from_array->data, from_array->size * sizeof(vfs_dir_entry_p));
    memset(from_array->data, 0, from_array->size * sizeof(vfs_dir_entry_p));
    to_array->size += from_array->size;
    from_array->size = 0;
    return ERROR_SUCCESS;
}

int is_big_endian()
{
    unsigned int value = 0xAA;
    unsigned char *ptr = (unsigned char *)&value;
    if (*ptr == 0xAA)
    {
        return 0;
    }
    return 1;
}

unsigned int change_endian(unsigned int x)
{
    static int endian = -1;
    if (endian == -1)
    {
        endian = is_big_endian();
    }
    if (endian)
    {
        return x;
    }
    unsigned char *ptr = (unsigned char *)&x;
    return (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
}

int vfs_unpack_files(FILE *file, vfs_dir_entry_array_p dirs,vfs_dir_entry_array_p files)
{
    size_t read_size = 0;
    int result = ERROR_INVALID_VFS;
    vfs_dir_entry_p entry = NULL;
    unsigned char name_size = 0;
    unsigned char end_node[NODE_SIZE] = {0};
    unsigned char buf[NODE_SIZE];

    do
    {
        read_size = fread(buf, 1, NODE_SIZE, file);
        if (read_size != NODE_SIZE)
        {
            break;
        }
        if (memcmp(buf, end_node, NODE_SIZE) == 0)
        {
            result = ERROR_SUCCESS;
            break;
        }
        name_size = buf[NODE_SIZE - 1];
        entry = (vfs_dir_entry_p)malloc(sizeof(vfs_dir_entry_t) + name_size);
        if (entry == NULL)
        {
            result = ERROR_NO_MEMORY;
            break;
        }
        memcpy(entry, buf, NODE_SIZE);
        entry->id = change_endian(entry->id);
        entry->file_size = change_endian(entry->file_size);
#ifdef __USE_ZLIB__
        entry->compress_size = change_endian(entry->compress_size);
#endif
        entry->offset = 0;
        entry->name[0] = 0;
        if (name_size > 0)
        {
            read_size = fread(entry->name, 1, name_size, file);
            if (read_size != name_size)
            {
                break;
            }
            entry->name[name_size] = 0;
        }
        if (IS_DIR(entry))
        {
            result = add_vfs_dir_entry(dirs, entry);
            if (result != ERROR_SUCCESS)
            {
                break;
            }
        }
        else
        {
            result = add_vfs_dir_entry(files, entry);
            if (result != ERROR_SUCCESS)
            {
                break;
            }
        }
    } while (1);
    return result;
}

int vfs_unpack(FILE *file, vfs_handle_p vfs)
{
    int result = 0;
    long file_size = 0, offset;
    vfs_dir_entry_array_t  dirs,files;

    if (0 != fseek(file, 0, SEEK_END))
    {
        return ERROR_INVALID_VFS;
    }
    file_size = ftell(file);
    if (0 != fseek(file, 0, SEEK_SET))
    {
        return ERROR_INVALID_VFS;
    }

    if(ERROR_SUCCESS != init_vfs_dir_entry_array(&dirs,1024) ){
        return ERROR_NO_MEMORY;
    }
    if(ERROR_SUCCESS != init_vfs_dir_entry_array(&files,1024) ){
        free_vfs_dir_entry_array(&dirs);
        return ERROR_NO_MEMORY;
    }
    do
    {
        result = vfs_unpack_files(file, &dirs,&files);
        if (result != ERROR_SUCCESS)
        {
            break;
        }
        result = move_dir_entry(&vfs->dirs,&dirs);
        if (result != ERROR_SUCCESS)
        {
            break;
        }
        offset = ftell(file);
        for (int i = 0; i < files.size; i++)
        {
            if (files.data[i]->id > vfs->dirs.size)
            {
                result = ERROR_INVALID_VFS;
                break;
            }
            files.data[i]->offset = offset;
#ifdef __USE_ZLIB__
            offset += (long)(files.data[i]->compress_size);
#else
            offset += (long)(files.data[i]->file_size);
#endif
        }
    } while (offset < file_size);
    return result;
}

void vfs_close(vfs_handle_p vfs)
{
    if (vfs == NULL)
    {
        return;
    }
    free_vfs_dir_entry_array(&vfs->dirs);
    free_vfs_dir_entry_array(&vfs->files);
    if (vfs->host_file != NULL)
    {
        fclose(vfs->host_file);
        vfs->host_file = NULL;
    }
    mutext_destory(&vfs->lock);
#ifdef __USE_ZLIB__
    if (vfs->dic != NULL)
    {
        free(vfs->dic);
        vfs->dic = NULL;
    }
#endif
    free(vfs);
}
#ifdef __USE_ZLIB__
//open a vfs object
vfs_handle_p vfs_open(const char *host_file_path, void *dic, int dic_size)
#else
vfs_handle_p vfs_open(const char *host_file_path)
#endif
{
    FILE *host_file = NULL;
    vfs_handle_p vfs = NULL;
    int result = ERROR_NO_MEMORY;
    long offset = 0;

    host_file = fopen(host_file_path, "rb");
    if (host_file == NULL)
    {
        return NULL;
    }
    do
    {
        vfs = (vfs_handle_p)malloc(sizeof(vfs_handle_t));
        if (vfs == NULL)
        {
            break;
        }
        mutext_init(&vfs->lock, NULL);
        memset(vfs, 0, sizeof(vfs_handle_t));
        result = init_vfs_dir_entry_array(&vfs->dirs, 4096);
        if (result != 0)
        {
            break;
        }
        result = init_vfs_dir_entry_array(&vfs->files, 4096);
        if (result != 0)
        {
            break;
        }
#ifdef __USE_ZLIB__
        vfs->dic = malloc(dic_size);
        if (vfs->dic == NULL)
        {
            break;
        }
        memcpy(vfs->dic, dic, dic_size);
        vfs->dic_size = dic_size;
#endif
        vfs->host_file = NULL;
        result = vfs_unpack_files(host_file, &vfs->dirs,&vfs->files);
        if (result != ERROR_SUCCESS)
        {
            break;
        }
        offset = ftell(host_file);
        for (int i = 0; i < vfs->files.size; i++)
        {
            if (vfs->files.data[i]->id > vfs->dirs.size)
            {
                result = ERROR_INVALID_VFS;
                break;
            }
            vfs->files.data[i]->offset = offset;
#ifdef __USE_ZLIB__
            offset += (long)(vfs->files.data[i]->compress_size);
#else
            offset += (long)(vfs->files.data[i]->file_size);
#endif
        }
    } while (0);
    if (result == ERROR_SUCCESS)
    {
        vfs->host_file = host_file;
        return vfs;
    }
    if (vfs != NULL)
    {
        vfs_close(vfs);
    }
    if (host_file != NULL)
    {
        fclose(host_file);
    }
    return NULL;
}

const vfs_dir_entry_p *vfs_get_all_files(vfs_handle_p vfs, int *count)
{
    *count = vfs->files.size;
    return vfs->files.data;
}

int vfs_get_file_full_path(vfs_handle_p vfs,
                           vfs_dir_entry_p entry,
                           char *full_path, int *path_size)
{
    int need_size = 0;
    int addSlip = 0;
    if (entry->id >= vfs->dirs.size)
    {
        return ERROR_INVALID_PARAMETER;
    }
    vfs_dir_entry_p parent = vfs->dirs.data[GET_PARENT_ID(entry)];
    need_size = strlen(parent->name);
    if (need_size > 0)
    {
        if (parent->name[need_size - 1] != '/')
        {
            need_size += 1;
            addSlip = 1;
        }
    }
    need_size += strlen(entry->name);
    if (need_size > *path_size)
    {
        *path_size = need_size;
        return ERROR_BUFFER_OVERFLOW;
    }
    strcpy(full_path, parent->name);
    if (addSlip)
    {
        strcat(full_path, "/");
    }
    strcat(full_path, entry->name);
    *path_size = need_size;
    return ERROR_SUCCESS;
}

int vfs_lookup_name_helper(vfs_dir_entry_array_p array, const char *name)
{
    int i = 0, j = array->size, h, result = 0;
    while (i <= j)
    {
        h = (int)((unsigned int)(i + j) >> 1);
        result = strcmp(name, array->data[h]->name);
        if (result == 0)
        {
            return h;
        }
        else if (result < 0)
        {
            j = h;
        }
        else
        {
            i = h + 1;
        }
    }
    return -1;
}

const vfs_dir_entry_p
vfs_lookup(vfs_handle_p vfs, const char *full_path)
{
    int root = 0;
    const char *base = NULL;
    vfs_dir_entry_p entry = NULL;
    char retail_path[MAX_PATH] = {0};
    char *cmp_ptr = &retail_path[0];
    int path_size = 0, index = -1;
    base = strrchr(full_path, '/');
    if (base == NULL)
    {
        base = full_path;
        root = 1;
    }
    else
    {
        if (base == full_path)
        {
            root = 1;
        }
        base++;
    }
    if (*full_path != '/')
    {
        cmp_ptr++;
    }
    index = vfs_lookup_name_helper(&vfs->files, base);
    if (index == -1)
    {
        return NULL;
    }
    for (int i = index; i > 0; i--)
    {
        entry = vfs->files.data[i];
        if (strcmp(entry->name, base))
        {
            break;
        }
        if (root)
        {
            if (GET_PARENT_ID(entry) == 0)
            {
                return entry;
            }
            continue;
        }
        path_size = sizeof(retail_path);
        if (0 != vfs_get_file_full_path(vfs, entry, retail_path, &path_size))
        {
            return NULL;
        }

        if (strcmp(cmp_ptr, full_path) == 0)
        {
            return entry;
        }
    }
    for (int i = index; i < vfs->files.size; i++)
    {
        entry = vfs->files.data[i];
        if (strcmp(entry->name, base))
        {
            break;
        }
        if (root)
        {
            if (GET_PARENT_ID(entry) == 0)
            {
                return entry;
            }
            continue;
        }
        path_size = sizeof(retail_path);
        if (0 != vfs_get_file_full_path(vfs, entry, retail_path, &path_size))
        {
            return NULL;
        }

        if (strcmp(cmp_ptr, full_path) == 0)
        {
            return entry;
        }
    }
    return NULL;
}

int vfs_raw_read_at(vfs_handle_p vfs,
                    void *buf,
                    long offset,
                    int size)
{
    int error = -1;
    if (size == 0)
    {
        return 0;
    }
    mutext_lock(&vfs->lock);
    do
    {
        if (0 != fseek(vfs->host_file, offset, SEEK_SET))
        {
            break;
        }
        error = fread(buf, 1, size, vfs->host_file);
    } while (0);
    mutext_unlock(&vfs->lock);
    return error;
}

#ifdef __USE_ZLIB__
void *
vfs_get_file_all_content(vfs_handle_p vfs,
                         vfs_dir_entry_p entry, int *error_p)
{
    void *uncompress_buf, *raw;
    int error = ERROR_NO_MEMORY;
    z_stream zs = {0};
    do
    {
        raw = malloc(entry->compress_size);
        uncompress_buf = malloc(entry->file_size);
        if (raw == NULL || uncompress_buf == NULL)
        {
            error = ERROR_NO_MEMORY;
            break;
        }
        if (entry->compress_size != vfs_raw_read_at(vfs, raw, entry->offset, entry->compress_size))
        {
            error = ERROR_INVALID_VFS;
            break;
        }
        inflateInit(&zs);

        zs.avail_in = entry->compress_size;
        zs.next_in = raw;
        zs.next_out = uncompress_buf;
        zs.avail_out = entry->file_size;
        error = inflate(&zs, Z_FINISH);

        if (error != Z_NEED_DICT)
        {
            error = ERROR_INVALID_VFS;
            break;
        }
        error = inflateSetDictionary(&zs, vfs->dic, vfs->dic_size);
        if (error != Z_OK)
        {
            error = ERROR_INVALID_VFS;
            break;
        }
        error = inflate(&zs, Z_FINISH);
        if (error != Z_STREAM_END)
        {
            error = ERROR_INVALID_VFS;
            break;
        }
        error = ERROR_SUCCESS;
    } while (0);

    inflateEnd(&zs);
    if (raw != NULL)
    {
        free(raw);
    }
    if (error_p)
        *error_p = error;
    if (error == ERROR_SUCCESS)
    {
        return uncompress_buf;
    }
    if (uncompress_buf != NULL)
    {
        free(uncompress_buf);
    }
    return NULL;
}

#else
void *
vfs_get_file_content(vfs_handle_p vfs,
                     vfs_dir_entry_p entry,
                     int offset,
                     int size,
                     int *read_size,
                     int *error)
{
    void *content = NULL;
    if (offset > entry->file_size)
    {
        if (error)
        {
            *error = ERROR_INVALID_PARAMETER;
        }
        return NULL;
    }
    if (offset + size > entry->file_size)
    {
        size = entry->file_size - offset;
    }

    content = malloc(size);
    if (content == NULL)
    {
        if (error)
        {
            *error = ERROR_NO_MEMORY;
        }
        return NULL;
    }
    if (size != vfs_raw_read_at(vfs, content, entry->offset + offset, size))
    {
        if (error)
        {
            *error = ERROR_INVALID_VFS;
        }
        free(content);
        return NULL;
    }
    if (read_size)
    {
        *read_size = size;
    }
    return content;
}

void *vfs_get_file_all_content(vfs_handle_p vfs,
                               vfs_dir_entry_p entry,
                               int *error)
{
    return vfs_get_file_content(vfs, entry, 0, entry->file_size, NULL, error);
}

#endif

void *vfs_read_file_content(const char *path, int *dic_size)
{
    FILE *f = NULL;
    void *content = NULL;
    size_t size = 0, read_size = 0;
    f = fopen(path, "r");
    if (f == NULL)
    {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    content = malloc(size);
    if (content == NULL)
    {
        fclose(f);
        return NULL;
    }
    read_size = fread(content, 1, size, f);
    fclose(f);
    if (read_size != size)
    {
        free(content);
        return NULL;
    }
    *dic_size = read_size;
    return content;
}
/*
int main()
{
    vfs_handle_p vfs = NULL;
    const vfs_dir_entry_p *list = NULL;
    vfs_dir_entry_p entry = NULL;
    char *content = NULL;
    int count = 0, dic_size = 0;
    do
    {
#ifdef __USE_ZLIB__
        content = vfs_read_file_content("/Volumes/work/tmp/dic.dat", &dic_size);
        if (content == NULL)
        {
            printf("load dic error\n");
            break;
        }
        vfs = vfs_open("/Volumes/work/tmp/pack.pf", content, dic_size);
        free(content);
#else
        vfs = vfs_open("/Volumes/work/tmp/uncompresspack.pf");
#endif
        if (vfs == NULL)
        {
            printf("vfs_open error\n");
            break;
        }
        list = vfs_get_all_files(vfs, &count);
        printf("vfs: file count %d\n", count);
        entry = vfs_lookup(vfs, "/404.inc");
        if (entry != NULL)
        {
            content = vfs_get_file_all_content(vfs, entry, NULL);
            if (content != NULL)
            {
                printf("%s\n", content);
                vfs_free_file_content(content);
            }
        }
        entry = vfs_lookup(vfs, "2021/gb_tlsv10_v11_detect.nasl");
        if (entry != NULL)
        {
            content = vfs_get_file_all_content(vfs, entry, NULL);
            if (content != NULL)
            {
                printf("%s\n", content);
                vfs_free_file_content(content);
            }
        }
    } while (0);
    if (vfs != NULL)
    {
        vfs_close(vfs);
    }
    return 0;
}*/