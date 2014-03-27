/***************************************************************************
 * 
 * Copyright (c) 2014 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 /**
 * @file tera_easy.h
 * @author yanshiguang02@baidu.com
 * @date 2014/02/04 16:35:14
 * @brief tera easy api
 *  
 **/

#ifndef  TERA_TERA_EASY_H_
#define  TERA_TERA_EASY_H_

#include <map>
#include <string>

namespace teraeasy {

typedef std::string Key;
typedef std::map<int64_t, std::string> Column;
typedef std::map<Key, Column> Record;
typedef std::map<Key, Record> TableSlice;

class Table {
public:
    Table() {}
    virtual ~Table() {}
    virtual bool Read(const Key& row_key, Record* record) = 0;
    virtual bool Write(const Key& row_key, const Record& record) = 0;
    virtual bool Delete(const Key& row_key) = 0;
    virtual bool Scan(const Key& start, const Key& end, TableSlice* slice) = 0;
private:
    Table(const Table&);
    void operator=(const Table&);
};

Table* OpenTable(const std::string& table_name);

}

#endif  //__TERA_EASY_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
