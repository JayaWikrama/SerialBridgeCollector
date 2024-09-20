#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "virtual-proxy.hpp"
#include "sqlite3-log.hpp"

#define ALLOW_DUPLICATE_LOG 0
#define LIMIT_DUP 1

struct info_t {
    std::string logName;
    std::string deviceName;
    std::string phyName;
};

void displayData(const std::vector <unsigned char> &data){
    for (auto i = data.begin(); i < data.end(); ++i){
        std::cout << std::hex << (int) *i;
        std::cout << " ";
    }
    std::cout << std::endl;
}

void passthroughFunc(Serial &src, Serial &dest, void *param){
    static std::vector <unsigned char> dataPhy;
    static std::vector <unsigned char> dataPseudo;
    static int counterSimPhy = 0;
    static int counterSimPseudo = 0;
    std::vector <unsigned char> data;
    struct timeval tv;
    struct info_t *info = (struct info_t *) param;
    Sqlite3LogSBColl log(info->logName, info->deviceName, 200);
    if (src.readData() == 0){
        data = src.getBufferAsVector();
        if (ALLOW_DUPLICATE_LOG == 0){
            if (info->phyName == src.getPort()){
                if (dataPhy.size() == data.size()){
                    if (memcmp(dataPhy.data(), data.data(), data.size()) == 0){
                        counterSimPhy++;
                        if (counterSimPhy > LIMIT_DUP) goto next;
                        goto loging;
                    }
                }
                dataPhy.assign(data.begin(), data.end());
                counterSimPhy = 0;
            }
            else {
                if (dataPseudo.size() == data.size()){
                    if (memcmp(dataPseudo.data(), data.data(), data.size()) == 0){
                        counterSimPseudo++;
                        if (counterSimPseudo > LIMIT_DUP) goto next;
                        goto loging;
                    }
                }
                dataPseudo.assign(data.begin(), data.end());
                counterSimPseudo = 0;
            }
        }
loging:
        gettimeofday(&tv, nullptr);
        log.insertLog(&tv, (info->phyName == src.getPort() ? true : false), data);
next:
        std::cout << src.getPort() << " >>> " << dest.getPort() << " [sz=" << std::to_string(data.size()) << "] : ";
        displayData(data);
        dest.writeData(data);
    }
}

int main(int argc, char **argv){
    if (argc != 5){
        std::cout << "cmd: " << argv[0] << " <physicalPort> <symlinkPort> <deviceName> <logName>" << std::endl;
        exit(0);
    }
    struct info_t info = {
        std::string(argv[4]),
        std::string(argv[3]),
        std::string(argv[1])
    };
    Sqlite3LogSBColl log(info.logName);
    log.createLog();
    VirtualSerialProxy proxy(argv[1], argv[2], B9600);
    proxy.setPassThrough(&passthroughFunc, (void *) &info);
    proxy.begin();
    return 0;
}
