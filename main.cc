#include <stdio.h>

#include <fstream>

#include "engine/parser.hpp"
#include "engine/vm.hpp"
using namespace Interpreter;
#include "engine/script.lex.hpp"
#include "engine/script.tab.hpp"
#include "modules/modules.h"
void yyerror(Interpreter::Parser* parser, const char* s) {
    printf("%s on line:%d\n", s, yylineno);
}

char* read_file_content(const char* path, int* file_size) {
    FILE* f = NULL;
    char* content = NULL;
    size_t size = 0, read_size = 0;
    f = fopen(path, "r");
    if (f == NULL) {
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    content = (char*)malloc(size + 2);
    if (content == NULL) {
        fclose(f);
        return NULL;
    }
    read_size = fread(content, 1, size, f);
    fclose(f);
    if (read_size != size) {
        free(content);
        return NULL;
    }
    *file_size = read_size;
    content[read_size] = 0;
    content[read_size + 1] = 0;
    return content;
}

scoped_refptr<Script> ParserFile(std::string path) {
    YY_BUFFER_STATE bp;
    char* context;
    int file_size;
    context = read_file_content(path.c_str(), &file_size);
    if (context == NULL) {
        return NULL;
    }
    scoped_refptr<Parser> parser = make_scoped_refptr(new Parser());
    bp = yy_scan_buffer(context, file_size + 2);
    yy_switch_to_buffer(bp);
    parser->Start(split(path, '/').back());
    int error = yyparse(parser.get());
    yy_flush_buffer(bp);
    yy_delete_buffer(bp);
    yylex_destroy();
    free(context);
    if (error) {
        return NULL;
    }
    return parser->Finish();
}

class DefaultExecutorCallback : public ExecutorCallback {
protected:
    std::string mFolder;

public:
    DefaultExecutorCallback(const std::string& folder) : mFolder(folder) {}
    scoped_refptr<Script> LoadScript(const char* name) {
        std::string path = mFolder + name;
        return ParserFile(path);
    }
};

int main(int argc, char* argv[]) {
    std::string folder = argv[1];
    folder = folder.substr(0, folder.rfind('/') + 1);
    scoped_refptr<Script> script = ParserFile(argv[1]);
    if (script != NULL) {
        DefaultExecutorCallback callback(folder);
        Executor exe(&callback);
        std::string err = "";
        RegisgerModulesBuiltinMethod(&exe);
        //std::cout << script->DumpInstruction(script->EntryPoint,"")<<std::endl;
        if (!exe.Execute(script, err, false)) {
            fprintf(stderr, "execute error:%s\n", err.c_str());
            return -1;
        }
        return 0;
    }
    return -1;
}