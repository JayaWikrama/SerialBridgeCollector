/*
 * $Id: sqlite3-log.hpp,v 1.0.0 2024/09/19 12:51:47 Jaya Wikrama Exp $
 *
 * Copyright (c) 2024 Jaya Wikrama
 * jayawikrama89@gmail.com
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

/**
 * @file
 * @brief SQLITE3 Implementation For Loging Serial Bridge Collection Data.
 *
 * This file contains a collection of functions and commands designed to facilitate and extend
 * basic sqlite3 function for specific purpose: Loging Serial Bridge Collection Data.
 *
 * @note This file is a part of a larger project focusing on enhancing collecting serial data
 *       by create serial proxy/bridge.
 *
 * @version 1.0.0
 * @date 2024-09-19
 * @author Jaya Wikrama
 */

#ifndef __SQLITE3_LOG_HPP__
#define __SQLITE3_LOG_HPP__

#include <pthread.h>
#include <string>
#include <vector>

class Sqlite3LogSBColl {
  private:
    unsigned short keepSz;  /*!< variable used to maintain the maximum size of the log */
    std::string device;     /*!< variable used to declare the identity of the connected serial device */
    std::string file;       /*!< sqlite3 database file name */
    pthread_mutex_t mtx;    /*!< for safe threading */

  public:

    /**
     * @brief Default constructor.
     *
     * This constructor initializes private data members and parameters to their default values, including:
     * `keepSz` = 30 MB
     * `device` = common
     * `file` = SBColl.log
     * Initialize mutex
     */
    Sqlite3LogSBColl();

    /**
     * @brief Custom constructor.
     *
     * This constructor initializes private data members and parameters to their default values except for log file name.
     *
     * @param[in] file sqlite3 database file name.
     */
    Sqlite3LogSBColl(const std::string file);

    /**
     * @brief Custom constructor.
     *
     * This constructor initializes private data members and parameters to their default values except for log file name and device name.
     *
     * @param[in] file sqlite3 database file name.
     * @param[in] device device identifier.
     */
    Sqlite3LogSBColl(const std::string file, const std::string device);

    /**
     * @brief Custom constructor.
     *
     * This constructor initializes private data members and parameters to their default values except for log file name, device name, and limit size.
     *
     * @param[in] file sqlite3 database file name.
     * @param[in] device device identifier.
     * @param[in] keepSz variable used to maintain the maximum size of the log.
     */
    Sqlite3LogSBColl(const std::string file, const std::string device, unsigned short keepSz);

    /**
     * @brief Destructor.
     *
     * This destructor is responsible for releasing any memory that has been allocated during the object's lifetime.
     * It ensures that all allocated resources are properly freed, preventing memory leaks.
     */
    ~Sqlite3LogSBColl();

    /**
     * @brief Create Log.
     *
     * This method is responsible for creating the log file (only called once).
     *
     * @return 0 if success.
     * @return 1 if failed to open sqlite3 descriptor.
     * @return 2 if failed to execute sqlite3.
     */
    int createLog();

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
    int insertLog(const struct timeval *tv, bool isPhy, const std::vector <unsigned char> data);

    /**
     * @brief Delete Log.
     *
     * This method is responsible for deleting logs based on the time limits specified in the function parameters.
     * @param[in] nDaysOlder All logs created before the number of days represented by this variable will be deleted.
     * @return 0 if success.
     * @return 1 if failed to open sqlite3 descriptor.
     * @return 2 if failed to execute sqlite3.
     */
    int deleteLog(unsigned int nDaysOlder);

    /**
     * @brief Keep the log size from exceeding the limit.
     *
     * Excess logs will be deleted automatically.
     */
    void maintainLog();
};

#endif