#include <iostream>
#include <string.h>
#include <unistd.h>
#include <sqlite3.h>
#include <fstream>
#include "sqlite3-log.hpp"

int trySqlite3Execute(int tryTimes, sqlite3 **db, const char *cmd){
    int sqlite3Ret = SQLITE_OK;
    char *errMsg = nullptr;
    do {
        sqlite3Ret = sqlite3_exec(*db, cmd, 0, 0, &errMsg);
        if (sqlite3Ret == SQLITE_BUSY || sqlite3Ret == SQLITE_LOCKED){
            sqlite3_sleep(125);
        }
        else if (sqlite3Ret == SQLITE_OK){
            return 0;
        }
        else if (memcmp(cmd, "CREATE", 6) == 0 ||
                 memcmp(cmd, "Create", 6) == 0 ||
                 memcmp(cmd, "create", 6) == 0
        ){
            return 0;
        }
        else {
            std::cerr << __func__ << ": " << errMsg << std::endl;
            std::cerr << cmd << std::endl;
            sqlite3_free(errMsg);
            break;
        }
        tryTimes--;
    } while (tryTimes > 0);
    return 1;
}

std::string base16Encoding(const std::vector <unsigned char> data){
    char tmp[3];
    std::string result;
    tmp[2] = 0x00;
    for (int i = 0; i < data.size(); i++){
        sprintf(tmp, "%02X", data[i]);
        result += tmp;
    }
    return result;
}

/**
 * @brief Default constructor.
 *
 * This constructor initializes private data members and parameters to their default values, including:
 * `keepSz` = 30 MB
 * `device` = common
 * `file` = SBColl.log
 * Initialize mutex
 */
Sqlite3LogSBColl::Sqlite3LogSBColl(){
    this->keepSz = 30;
    this->device = "common";
    this->file = "SBColl.log";
    pthread_mutex_init(&(this->mtx), nullptr);
}

/**
 * @brief Custom constructor.
 *
 * This constructor initializes private data members and parameters to their default values except for log file name.
 *
 * @param[in] file sqlite3 database file name.
 */
Sqlite3LogSBColl::Sqlite3LogSBColl(const std::string file){
    this->keepSz = 30;
    this->device = "common";
    this->file = file;
    pthread_mutex_init(&(this->mtx), nullptr);
}

/**
 * @brief Custom constructor.
 *
 * This constructor initializes private data members and parameters to their default values except for log file name and device name.
 *
 * @param[in] file sqlite3 database file name.
 * @param[in] device device identifier.
 */
Sqlite3LogSBColl::Sqlite3LogSBColl(const std::string file, const std::string device){
    this->keepSz = 30;
    this->device = device;
    this->file = file;
    pthread_mutex_init(&(this->mtx), nullptr);
}

/**
 * @brief Custom constructor.
 *
 * This constructor initializes private data members and parameters to their default values except for log file name, device name, and limit size.
 *
 * @param[in] file sqlite3 database file name.
 * @param[in] device device identifier.
 * @param[in] keepSz variable used to maintain the maximum size of the log.
 */
Sqlite3LogSBColl::Sqlite3LogSBColl(const std::string file, const std::string device, unsigned short keepSz){
    this->keepSz = keepSz;
    this->device = device;
    this->file = file;
    pthread_mutex_init(&(this->mtx), nullptr);
}

/**
 * @brief Destructor.
 *
 * This destructor is responsible for releasing any memory that has been allocated during the object's lifetime.
 * It ensures that all allocated resources are properly freed, preventing memory leaks.
 */
Sqlite3LogSBColl::~Sqlite3LogSBColl(){
    pthread_mutex_destroy(&(this->mtx));
}

/**
 * @brief Create Log.
 *
 * This method is responsible for creating the log file (only called once).
 *
 * @return 0 if success.
 * @return 1 if failed to open sqlite3 descriptor.
 * @return 2 if failed to execute sqlite3.
 */
int Sqlite3LogSBColl::createLog(){
    pthread_mutex_lock(&(this->mtx));
    int retry = 5;
    sqlite3 *db = nullptr;
    if (sqlite3_open(this->file.c_str(), &db) != SQLITE_OK){
        if (db != nullptr){
            sqlite3_close(db);
            db = nullptr;
        }
        pthread_mutex_unlock(&(this->mtx));
        return 1;
    }
    sqlite3_busy_timeout(db, 5000);
    if (trySqlite3Execute(3, &db, "PRAGMA journal_mode=WAL;")){
        trySqlite3Execute(3, &db, "ROLLBACK;");
        sqlite3_close(db);
        db = nullptr;
        pthread_mutex_unlock(&(this->mtx));
        return 2;
    }
    if (trySqlite3Execute(3, &db, "BEGIN TRANSACTION;")){
        trySqlite3Execute(3, &db, "ROLLBACK;");
        sqlite3_close(db);
        db = nullptr;
        pthread_mutex_unlock(&(this->mtx));
        return 2;
    }
    std::string sql = "CREATE TABLE proxy (\n"
                      "                     id INTEGER PRIMARY KEY,    -- ID Counter\n"
                      "                     time INT,                  -- time in second\n"
                      "                     timeUs INT,                -- time in uSecond\n"
                      "                     type INT,                  -- 1 from phy, 0 to phy\n"
                      "                     device TEXT,               -- Device identifier\n"
                      "                     data TEXT                  -- data encoded in base 16\n"
                      ");";
    if (trySqlite3Execute(5, &db, sql.c_str())) {
        trySqlite3Execute(3, &db, "ROLLBACK;");
        sqlite3_close(db);
        db = nullptr;
        pthread_mutex_unlock(&(this->mtx));
        return 2;
    }
    if (trySqlite3Execute(3, &db, "COMMIT;")){
        trySqlite3Execute(3, &db, "ROLLBACK;");
        sqlite3_close(db);
        db = nullptr;
        pthread_mutex_unlock(&(this->mtx));
        return 2;
    }
    sqlite3_close(db);
    db = nullptr;
    pthread_mutex_unlock(&(this->mtx));
    return 0;
}

/**
 * @brief Insert Log.
 *
 * This method is responsible for writing logs.
 *
 * @param[in] isPhy Set variable ini bernilai true jika data berasal dari physical port.
 * @param[in] data serial data.
 * @return 0 if sukses.
 * @return 1 if failed to open sqlite3 descriptor.
 * @return 2 if failed to allocate memory for sqlite3 command.
 * @return 3 if failed to execute sqlite3 command.
 */
int Sqlite3LogSBColl::insertLog(const struct timeval *tv, bool isPhy, const std::vector <unsigned char> data){
    pthread_mutex_lock(&(this->mtx));
    int retry = 5;
    sqlite3 *db = nullptr;
    if (sqlite3_open(this->file.c_str(), &db) != SQLITE_OK){
        if (db != nullptr){
            sqlite3_close(db);
            db = nullptr;
        }
        pthread_mutex_unlock(&(this->mtx));
        return 1;
    }
    sqlite3_busy_timeout(db, 5000);
    if (trySqlite3Execute(3, &db, "PRAGMA journal_mode=WAL;")){
        trySqlite3Execute(3, &db, "ROLLBACK;");
        sqlite3_close(db);
        db = nullptr;
        pthread_mutex_unlock(&(this->mtx));
        return 2;
    }
    if (trySqlite3Execute(3, &db, "BEGIN TRANSACTION;")){
        trySqlite3Execute(3, &db, "ROLLBACK;");
        sqlite3_close(db);
        db = nullptr;
        pthread_mutex_unlock(&(this->mtx));
        return 2;
    }
    std::string sql = std::string("INSERT INTO proxy ("
                                    "time,"
                                    "timeUs,"
                                    "type,"
                                    "device,"
                                    "data"
                                  ") VALUES (") +
                                    std::to_string(tv->tv_sec) + std::string(", ") +
                                    std::to_string(tv->tv_usec) + std::string(", ") +
                                    std::to_string((isPhy == true ? 1 : 0)) + std::string(", '") +
                                    this->device + std::string("', '") +
                                    base16Encoding(data) +
                                  std::string("');");
    if (trySqlite3Execute(5, &db, sql.c_str())) {
        trySqlite3Execute(3, &db, "ROLLBACK;");
        sqlite3_close(db);
        db = nullptr;
        pthread_mutex_unlock(&(this->mtx));
        return 2;
    }
    if (trySqlite3Execute(3, &db, "COMMIT;")){
        trySqlite3Execute(3, &db, "ROLLBACK;");
        sqlite3_close(db);
        db = nullptr;
        pthread_mutex_unlock(&(this->mtx));
        return 2;
    }
    sqlite3_close(db);
    db = nullptr;
    pthread_mutex_unlock(&(this->mtx));
    return 0;
}

/**
 * @brief Delete Log.
 *
 * This method is responsible for deleting logs based on the time limits specified in the function parameters.
 * @param[in] nDaysOlder All logs created before the number of days represented by this variable will be deleted.
 * @return 0 if success.
 * @return 1 if failed to open sqlite3 descriptor.
 * @return 2 if failed to execute sqlite3.
 */
int Sqlite3LogSBColl::deleteLog(unsigned int nDaysOlder){
    pthread_mutex_lock(&(this->mtx));
    sqlite3 *db = NULL;
    char sql[64];
    time_t currentTime = 0;
    time_t limitedTime = 0;
    struct tm tmHm;

    currentTime = time(NULL);
    limitedTime = currentTime - (nDaysOlder * 24 * 3600);
    memcpy(&tmHm, localtime(&limitedTime), sizeof(tmHm));
    if (tmHm.tm_hour >= 2030){
        pthread_mutex_unlock(&(this->mtx));
        return 4;
    }
    tmHm.tm_hour = 0;
    tmHm.tm_min = 0;
    tmHm.tm_sec = 0;
    limitedTime = mktime(&tmHm);
    
    if (sqlite3_open(this->file.c_str(), &db) != SQLITE_OK){
        if (db != NULL){
            sqlite3_close(db);
            db = NULL;
        }
        pthread_mutex_unlock(&(this->mtx));
        return 1;
    }
    sqlite3_busy_timeout(db, 5000);
    if (trySqlite3Execute(3, &db, "PRAGMA journal_mode=WAL;")){
        trySqlite3Execute(3, &db, "ROLLBACK;");
        sqlite3_close(db);
        db = nullptr;
        pthread_mutex_unlock(&(this->mtx));
        return 2;
    }
    if (trySqlite3Execute(3, &db, "BEGIN TRANSACTION;")){
        trySqlite3Execute(3, &db, "ROLLBACK;");
        sqlite3_close(db);
        db = nullptr;
        pthread_mutex_unlock(&(this->mtx));
        return 2;
    }
    memset(sql, 0x00, sizeof(sql));
    sprintf(sql, "DELETE FROM devLog WHERE time < %li;", static_cast<long>(limitedTime));
    if (trySqlite3Execute(5, &db, sql)) {
        trySqlite3Execute(3, &db, "ROLLBACK;");
        sqlite3_close(db);
        db = nullptr;
        pthread_mutex_unlock(&(this->mtx));
        return 2;
    }
    sqlite3_exec(db, "VACUUM;", 0, 0, NULL);
    if (trySqlite3Execute(3, &db, "COMMIT;")){
        trySqlite3Execute(3, &db, "ROLLBACK;");
        sqlite3_close(db);
        db = nullptr;
        pthread_mutex_unlock(&(this->mtx));
        return 2;
    }
    sqlite3_close(db);
    db = NULL;
    pthread_mutex_unlock(&(this->mtx));
    return 0;
}

/**
 * @brief Keep the log size from exceeding the limit.
 *
 * Excess logs will be deleted automatically.
 */
void Sqlite3LogSBColl::maintainLog(){
    int fSz = 0;
    pthread_mutex_lock(&(this->mtx));
    unsigned int ndays = 60;
    pthread_mutex_unlock(&(this->mtx));
    do {
        this->deleteLog(ndays);
        ndays--;
        pthread_mutex_lock(&(this->mtx));
        std::ifstream lFile(this->file, std::ifstream::binary);
        if (lFile){
            lFile.seekg(0, lFile.end);
            fSz = lFile.tellg() /  1000000;
        }
        pthread_mutex_unlock(&(this->mtx));
        usleep(125000);
    } while (fSz > this->keepSz);
}