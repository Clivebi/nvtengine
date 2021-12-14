#include <dirent.h>
#include <stdio.h>
#include <unistd.h>

#include <fstream>
#include <regex>

#include "./modules/openvas/api.hpp"
#include "engine/vm.hpp"
#include "modules/modules.h"
#include "modules/openvas/support/nvtidb.hpp"
#include "taskmgr.hpp"
std::string test_oids =
        "1.3.6.1.4.1.25623.1.0.100034;1.3.6.1.4.1.25623.1.0.10006;1.3.6.1.4.1.25623.1.0.100062;1.3."
        "6.1.4.1.25623.1.0.100069;1.3.6.1.4.1.25623.1.0.100074;1.3.6.1.4.1.25623.1.0.100082;1.3.6."
        "1.4.1.25623.1.0.100083;1.3.6.1.4.1.25623.1.0.10379;1.3.6.1.4.1.25623.1.0.100187;1.3.6.1.4."
        "1.25623.1.0.100254;1.3.6.1.4.1.25623.1.0.100259;1.3.6.1.4.1.25623.1.0.100280;1.3.6.1.4.1."
        "25623.1.0.100288;1.3.6.1.4.1.25623.1.0.100300;1.3.6.1.4.1.25623.1.0.100315;1.3.6.1.4.1."
        "25623.1.0.100329;1.3.6.1.4.1.25623.1.0.100331;1.3.6.1.4.1.25623.1.0.11762;1.3.6.1.4.1."
        "25623.1.0.100479;1.3.6.1.4.1.25623.1.0.100489;1.3.6.1.4.1.25623.1.0.10051;1.3.6.1.4.1."
        "25623.1.0.100517;1.3.6.1.4.1.25623.1.0.100558;1.3.6.1.4.1.25623.1.0.100651;1.3.6.1.4.1."
        "25623.1.0.100669;1.3.6.1.4.1.25623.1.0.100755;1.3.6.1.4.1.25623.1.0.100770;1.3.6.1.4.1."
        "25623.1.0.100780;1.3.6.1.4.1.25623.1.0.100819;1.3.6.1.4.1.25623.1.0.100838;1.3.6.1.4.1."
        "25623.1.0.10107;1.3.6.1.4.1.25623.1.0.100950;1.3.6.1.4.1.25623.1.0.101013;1.3.6.1.4.1."
        "25623.1.0.10147;1.3.6.1.4.1.25623.1.0.10150;1.3.6.1.4.1.25623.1.0.10159;1.3.6.1.4.1.25623."
        "1.0.10168;1.3.6.1.4.1.25623.1.0.10175;1.3.6.1.4.1.25623.1.0.10185;1.3.6.1.4.1.25623.1.0."
        "102001;1.3.6.1.4.1.25623.1.0.102003;1.3.6.1.4.1.25623.1.0.102005;1.3.6.1.4.1.25623.1.0."
        "102017;1.3.6.1.4.1.25623.1.0.102048;1.3.6.1.4.1.25623.1.0.10263;1.3.6.1.4.1.25623.1.0."
        "10273;1.3.6.1.4.1.25623.1.0.10281;1.3.6.1.4.1.25623.1.0.103125;1.3.6.1.4.1.25623.1.0."
        "103086;1.3.6.1.4.1.25623.1.0.103098;1.3.6.1.4.1.25623.1.0.103106;1.3.6.1.4.1.25623.1.0."
        "103123;1.3.6.1.4.1.25623.1.0.103147;1.3.6.1.4.1.25623.1.0.103156;1.3.6.1.4.1.25623.1.0."
        "103190;1.3.6.1.4.1.25623.1.0.103223;1.3.6.1.4.1.25623.1.0.10330;1.3.6.1.4.1.25623.1.0."
        "103326;1.3.6.1.4.1.25623.1.0.10335;1.3.6.1.4.1.25623.1.0.10342;1.3.6.1.4.1.25623.1.0."
        "10344;1.3.6.1.4.1.25623.1.0.10354;1.3.6.1.4.1.25623.1.0.103585;1.3.6.1.4.1.25623.1.0."
        "103652;1.3.6.1.4.1.25623.1.0.103835;1.3.6.1.4.1.25623.1.0.103854;1.3.6.1.4.1.25623.1.0."
        "103908;1.3.6.1.4.1.25623.1.0.103923;1.3.6.1.4.1.25623.1.0.103997;1.3.6.1.4.1.25623.1.0."
        "104000;1.3.6.1.4.1.25623.1.0.10407;1.3.6.1.4.1.25623.1.0.10742;1.3.6.1.4.1.25623.1.0."
        "10441;1.3.6.1.4.1.25623.1.0.10462;1.3.6.1.4.1.25623.1.0.105030;1.3.6.1.4.1.25623.1.0."
        "140170;1.3.6.1.4.1.25623.1.0.105420;1.3.6.1.4.1.25623.1.0.105421;1.3.6.1.4.1.25623.1.0."
        "105418;1.3.6.1.4.1.25623.1.0.105448;1.3.6.1.4.1.25623.1.0.105559;1.3.6.1.4.1.25623.1.0."
        "105488;1.3.6.1.4.1.25623.1.0.105489;1.3.6.1.4.1.25623.1.0.105565;1.3.6.1.4.1.25623.1.0."
        "105719;1.3.6.1.4.1.25623.1.0.105839;1.3.6.1.4.1.25623.1.0.105924;1.3.6.1.4.1.25623.1.0."
        "10595;1.3.6.1.4.1.25623.1.0.106127;1.3.6.1.4.1.25623.1.0.10617;1.3.6.1.4.1.25623.1.0."
        "10622;1.3.6.1.4.1.25623.1.0.106522;1.3.6.1.4.1.25623.1.0.10666;1.3.6.1.4.1.25623.1.0."
        "106693;1.3.6.1.4.1.25623.1.0.106716;1.3.6.1.4.1.25623.1.0.10674;1.3.6.1.4.1.25623.1.0."
        "10675;1.3.6.1.4.1.25623.1.0.10676;1.3.6.1.4.1.25623.1.0.106850;1.3.6.1.4.1.25623.1.0."
        "10730;1.3.6.1.4.1.25623.1.0.10731;1.3.6.1.4.1.25623.1.0.10758;1.3.6.1.4.1.25623.1.0.10761;"
        "1.3.6.1.4.1.25623.1.0.10762;1.3.6.1.4.1.25623.1.0.10796;1.3.6.1.4.1.25623.1.0.108018;1.3."
        "6.1.4.1.25623.1.0.108020;1.3.6.1.4.1.25623.1.0.10794;1.3.6.1.4.1.25623.1.0.108027;1.3.6.1."
        "4.1.25623.1.0.108044;1.3.6.1.4.1.25623.1.0.108082;1.3.6.1.4.1.25623.1.0.108090;1.3.6.1.4."
        "1.25623.1.0.108102;1.3.6.1.4.1.25623.1.0.108198;1.3.6.1.4.1.25623.1.0.108199;1.3.6.1.4.1."
        "25623.1.0.108203;1.3.6.1.4.1.25623.1.0.108204;1.3.6.1.4.1.25623.1.0.108215;1.3.6.1.4.1."
        "25623.1.0.108216;1.3.6.1.4.1.25623.1.0.108217;1.3.6.1.4.1.25623.1.0.108218;1.3.6.1.4.1."
        "25623.1.0.108219;1.3.6.1.4.1.25623.1.0.108220;1.3.6.1.4.1.25623.1.0.108299;1.3.6.1.4.1."
        "25623.1.0.108300;1.3.6.1.4.1.25623.1.0.108441;1.3.6.1.4.1.25623.1.0.108449;1.3.6.1.4.1."
        "25623.1.0.108477;1.3.6.1.4.1.25623.1.0.108478;1.3.6.1.4.1.25623.1.0.108479;1.3.6.1.4.1."
        "25623.1.0.108525;1.3.6.1.4.1.25623.1.0.108546;1.3.6.1.4.1.25623.1.0.108641;1.3.6.1.4.1."
        "25623.1.0.108696;1.3.6.1.4.1.25623.1.0.108708;1.3.6.1.4.1.25623.1.0.108731;1.3.6.1.4.1."
        "25623.1.0.108747;1.3.6.1.4.1.25623.1.0.108893;1.3.6.1.4.1.25623.1.0.108894;1.3.6.1.4.1."
        "25623.1.0.108896;1.3.6.1.4.1.25623.1.0.108897;1.3.6.1.4.1.25623.1.0.10891;1.3.6.1.4.1."
        "25623.1.0.10920;1.3.6.1.4.1.25623.1.0.10921;1.3.6.1.4.1.25623.1.0.10942;1.3.6.1.4.1.25623."
        "1.0.10990;1.3.6.1.4.1.25623.1.0.11011;1.3.6.1.4.1.25623.1.0.11016;1.3.6.1.4.1.25623.1.0."
        "11032;1.3.6.1.4.1.25623.1.0.11033;1.3.6.1.4.1.25623.1.0.111037;1.3.6.1.4.1.25623.1.0."
        "111045;1.3.6.1.4.1.25623.1.0.11111;1.3.6.1.4.1.25623.1.0.11120;1.3.6.1.4.1.25623.1.0."
        "11121;1.3.6.1.4.1.25623.1.0.11128;1.3.6.1.4.1.25623.1.0.11134;1.3.6.1.4.1.25623.1.0.11140;"
        "1.3.6.1.4.1.25623.1.0.11153;1.3.6.1.4.1.25623.1.0.11154;1.3.6.1.4.1.25623.1.0.11156;1.3.6."
        "1.4.1.25623.1.0.11219;1.3.6.1.4.1.25623.1.0.11239;1.3.6.1.4.1.25623.1.0.113756;1.3.6.1.4."
        "1.25623.1.0.113758;1.3.6.1.4.1.25623.1.0.113759;1.3.6.1.4.1.25623.1.0.113760;1.3.6.1.4.1."
        "25623.1.0.113763;1.3.6.1.4.1.25623.1.0.113765;1.3.6.1.4.1.25623.1.0.11414;1.3.6.1.4.1."
        "25623.1.0.11518;1.3.6.1.4.1.25623.1.0.11720;1.3.6.1.4.1.25623.1.0.117628;1.3.6.1.4.1."
        "25623.1.0.11913;1.3.6.1.4.1.25623.1.0.11822;1.3.6.1.4.1.25623.1.0.11865;1.3.6.1.4.1.25623."
        "1.0.11906;1.3.6.1.4.1.25623.1.0.11907;1.3.6.1.4.1.25623.1.0.11908;1.3.6.1.4.1.25623.1.0."
        "11929;1.3.6.1.4.1.25623.1.0.11945;1.3.6.1.4.1.25623.1.0.11963;1.3.6.1.4.1.25623.1.0.11986;"
        "1.3.6.1.4.1.25623.1.0.11987;1.3.6.1.4.1.25623.1.0.12638;1.3.6.1.4.1.25623.1.0.140050;1.3."
        "6.1.4.1.25623.1.0.140096;1.3.6.1.4.1.25623.1.0.140151;1.3.6.1.4.1.25623.1.0.140166;1.3.6."
        "1.4.1.25623.1.0.140215;1.3.6.1.4.1.25623.1.0.140247;1.3.6.1.4.1.25623.1.0.140278;1.3.6.1."
        "4.1.25623.1.0.140418;1.3.6.1.4.1.25623.1.0.140498;1.3.6.1.4.1.25623.1.0.140500;1.3.6.1.4."
        "1.25623.1.0.140512;1.3.6.1.4.1.25623.1.0.140538;1.3.6.1.4.1.25623.1.0.140597;1.3.6.1.4.1."
        "25623.1.0.140694;1.3.6.1.4.1.25623.1.0.141067;1.3.6.1.4.1.25623.1.0.141088;1.3.6.1.4.1."
        "25623.1.0.141116;1.3.6.1.4.1.25623.1.0.141120;1.3.6.1.4.1.25623.1.0.142077;1.3.6.1.4.1."
        "25623.1.0.142147;1.3.6.1.4.1.25623.1.0.14259;1.3.6.1.4.1.25623.1.0.14272;1.3.6.1.4.1."
        "25623.1.0.14274;1.3.6.1.4.1.25623.1.0.143229;1.3.6.1.4.1.25623.1.0.143507;1.3.6.1.4.1."
        "25623.1.0.14377;1.3.6.1.4.1.25623.1.0.14378;1.3.6.1.4.1.25623.1.0.145817;1.3.6.1.4.1."
        "25623.1.0.14663;1.3.6.1.4.1.25623.1.0.14664;1.3.6.1.4.1.25623.1.0.14674;1.3.6.1.4.1.25623."
        "1.0.14773;1.3.6.1.4.1.25623.1.0.14788;1.3.6.1.4.1.25623.1.0.15588;1.3.6.1.4.1.25623.1.0."
        "15765;1.3.6.1.4.1.25623.1.0.15766;1.3.6.1.4.1.25623.1.0.16363;1.3.6.1.4.1.25623.1.0.17583;"
        "1.3.6.1.4.1.25623.1.0.17975;1.3.6.1.4.1.25623.1.0.18183;1.3.6.1.4.1.25623.1.0.18184;1.3.6."
        "1.4.1.25623.1.0.18185;1.3.6.1.4.1.25623.1.0.18186;1.3.6.1.4.1.25623.1.0.18356;1.3.6.1.4.1."
        "25623.1.0.18373;1.3.6.1.4.1.25623.1.0.18393;1.3.6.1.4.1.25623.1.0.18528;1.3.6.1.4.1.25623."
        "1.0.18533;1.3.6.1.4.1.25623.1.0.18534;1.3.6.1.4.1.25623.1.0.19288;1.3.6.1.4.1.25623.1.0."
        "19289;1.3.6.1.4.1.25623.1.0.19608;1.3.6.1.4.1.25623.1.0.19689;1.3.6.1.4.1.25623.1.0.20160;"
        "1.3.6.1.4.1.25623.1.0.20301;1.3.6.1.4.1.25623.1.0.20377;1.3.6.1.4.1.25623.1.0.20834;1.3.6."
        "1.4.1.25623.1.0.66286;1.3.6.1.4.1.25623.1.0.80001;1.3.6.1.4.1.25623.1.0.80002;1.3.6.1.4.1."
        "25623.1.0.80003;1.3.6.1.4.1.25623.1.0.80004;1.3.6.1.4.1.25623.1.0.80005;1.3.6.1.4.1.25623."
        "1.0.80009;1.3.6.1.4.1.25623.1.0.80045;1.3.6.1.4.1.25623.1.0.80059;1.3.6.1.4.1.25623.1.0."
        "80062;1.3.6.1.4.1.25623.1.0.80063;1.3.6.1.4.1.25623.1.0.80064;1.3.6.1.4.1.25623.1.0.80066;"
        "1.3.6.1.4.1.25623.1.0.80079;1.3.6.1.4.1.25623.1.0.80092;1.3.6.1.4.1.25623.1.0.80095;1.3.6."
        "1.4.1.25623.1.0.800984;1.3.6.1.4.1.25623.1.0.80100;1.3.6.1.4.1.25623.1.0.801234;1.3.6.1.4."
        "1.25623.1.0.802244;1.3.6.1.4.1.25623.1.0.807830;1.3.6.1.4.1.25623.1.0.810002;1.3.6.1.4.1."
        "25623.1.0.810549;1.3.6.1.4.1.25623.1.0.810550;1.3.6.1.4.1.25623.1.0.810554;1.3.6.1.4.1."
        "25623.1.0.9000001;1.3.6.1.4.1.25623.1.0.900602;1.3.6.1.4.1.25623.1.0.901121;1.3.6.1.4.1."
        "25623.1.0.902447;1.3.6.1.4.1.25623.1.0.103680";

void CollectAllScript(FilePath path, FilePath relative_path, std::list<std::string>& result) {
    struct dirent* entry = NULL;
    DIR* dir = opendir(std::string(path).c_str());
    if (dir == NULL) {
        return;
    }
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type & DT_DIR) {
            if (entry->d_name[0] != '.') {
                CollectAllScript(path + entry->d_name, relative_path + entry->d_name, result);
            }
            continue;
        }
        if (entry->d_namlen > 0) {
            FilePath short_path = relative_path;
            short_path += entry->d_name;
            std::string element = short_path;
            if (element.find(".sc") != std::string::npos &&
                element.find(".inc.") == std::string::npos) {
                result.push_back(element);
            }
        }
    }
    closedir(dir);
}

void UpdateNVTI(std::string script_path, std::string home) {
    std::list<std::string> result;
    FileIO IO;
    CollectAllScript(script_path, "", result);
    Value ret = Value::make_array();
    for (auto iter : result) {
        OVAContext context(iter, Value::make_map(), Value::make_map(),
                           new support::ScriptStorage());
        DefaultExecutorCallback callback(script_path, &IO);
        callback.mDescription = true;
        Interpreter::Executor exe(&callback, &context);
        RegisgerModulesBuiltinMethod(&exe);
        bool ok = exe.Execute(iter.c_str(), false);
        if (ok) {
            ret._array().push_back(context.Nvti);
            if (ret.Length() > 5000) {
                support::NVTIDataBase db(FilePath(home) + "attributes.db");
                db.UpdateAll(ret);
                ret._array().clear();
            }
        }
        if (callback.mSyntaxError) {
            break;
        }
    }
    {
        support::NVTIDataBase db(FilePath(home) + "attributes.db");
        db.UpdateAll(ret);
        ret._array().clear();
    }
}

void NVTEngineTest() {
    Value pref = Value::make_map();
    //pref[knowntext::kPref_load_dependencies] = false;
    FileIO IO;
    pref["scripts_folder"] = "/Volumes/work/nvtscript";
    HostsTask task("192.168.0.106", "80,443", pref, &IO);
    std::list<std::string> result = Interpreter::split(test_oids, ';');
    std::list<std::string> list2;
    list2.push_back("1.3.6.1.4.1.25623.1.0.900234");
    task.BeginTask(result, "10000");
    task.Join();
}

int main(int argc, char* argv[]) {
#ifdef __APPLE__
    signal(SIGPIPE, SIG_IGN);
#endif
    std::cout << Interpreter::Status::ToString() << std::endl;
    if (argc >= 2) {
        UpdateNVTI(argv[1], "");
        std::cout << Interpreter::Status::ToString() << std::endl;
        return 0;
    }
    NVTEngineTest();
    std::cout << Interpreter::Status::ToString() << std::endl;
    return 0;
}