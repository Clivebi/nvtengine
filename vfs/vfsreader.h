
//if define __USE_ZLIB__ ,use the zlib compress 
//#define __USE_ZLIB__


#define ERROR_SUCCESS (0)
#define ERROR_NO_MEMORY (-1)
#define ERROR_INVALID_VFS (-2)
#define ERROR_BUFFER_OVERFLOW (-3)
#define ERROR_INVALID_PARAMETER (-4)

//opaque handle for manger vfs object
typedef struct _vfs_handle *vfs_handle_p;

//opaque handle for manger vfs file entry
typedef struct _vfs_dir_entry *vfs_dir_entry_p;

//clean up vfs
void vfs_close(vfs_handle_p vfs);

#ifdef __USE_ZLIB__
//open a vfs object
vfs_handle_p vfs_open(const char *host_file_path,void* dic,int dic_size);
#else
vfs_handle_p vfs_open(const char *host_file_path);
#endif

//get file size for entry present file
int vfs_file_size(vfs_dir_entry_p entry);

//get the file base name
const char*  vfs_file_name(vfs_dir_entry_p entry);

//get all files,count return the vfs_dir_entry_p array size
//return the vfs_dir_entry array
const vfs_dir_entry_p *vfs_get_all_files(vfs_handle_p vfs, int *count);

//get the fully file path for entry
//if the path_size < need size ,path_size = need size and return ERROR_BUFFER_OVERFLOW
//the max file path size is 512 byte
int vfs_get_file_full_path(vfs_handle_p vfs,
                           vfs_dir_entry_p entry,
                           char *full_path, int *path_size);

//look up one lined example
//full_path: 404.html  lookup 404.html file in root
//full_path: /404.html  lookup 404.html file in root
//full_path: /dir/404.html  lookup 404.html file in /dir/
//full_path: dir/404.html  lookup 404.html file in  /dir/
const vfs_dir_entry_p vfs_lookup(vfs_handle_p vfs,const char *full_path);


#ifdef __USE_ZLIB__
void *vfs_get_file_all_content(vfs_handle_p vfs,
                               vfs_dir_entry_p entry,
                               int *error);
#else
//get the file content from offset to offset+size
void *vfs_get_file_content(vfs_handle_p vfs,
                           vfs_dir_entry_p entry,
                           int offset,
                           int size,
                           int *read_size,
                           int *error);

//get the file content fully (from 0 to file size)
void *vfs_get_file_all_content(vfs_handle_p vfs,
                               vfs_dir_entry_p entry,
                               int *error);
#endif


#define vfs_free_file_content(content) free(content)

/* 
example
int main()
{
    vfs_handle_p vfs = NULL;
    const vfs_dir_entry_p *list = NULL;
    vfs_dir_entry_p entry = NULL;
    char *content = NULL;
    int count = 0,dic_size = 0;
    do
    {
#ifdef __USE_ZLIB__
        content = vfs_read_file_content("/Volumes/work/tmp/dic.dat",&dic_size);
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
}

*/