/***************************************************************************
 * 
 * Copyright (c) 2014, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 /**
 * @file tera_easy_sample.cc
 * @author yanshiguang02@baidu.com
 * @date 2014/02/04 18:21:59
 * @brief 
 *  
 **/

#include "tera_easy.h"

using teraeasy::Table;
using teraeasy::OpenTable;
using teraeasy::TableSlice;
using teraeasy::Record;
using teraeasy::Column;
using teraeasy::Key;

int main(int argc, char* argv[]) {
    // Open
    Table* table = teraeasy::OpenTable("WebTable");
    // Insert
    Record record;
    //table->Write("com.baidu.www/", record);
    // Read
    //table->Read("com.baidu.www/", &record);
    // Modify
    //record["anchor:www.hao123.com/"][time(NULL)] = "°Ù¶È";
    //table->Write("com.baidu.www/", record);
    // Scan
    std::string last_key = "";
    while(1) {
        TableSlice slice;
        table->Scan(last_key, "~", &slice);
        if (slice.empty()) {
            break;
        }
        for (TableSlice::const_iterator it = slice.begin(); it != slice.end(); ++it) {
            const Key& row_key = it->first;
            const Record& record= it->second;
            printf("Row key is %s\n", row_key.c_str());
            for (Record::const_iterator rit = record.begin(); rit != record.end(); ++rit) {
                const Key& column_key = rit->first;
                const Column& column = rit->second;
                for (Column::const_iterator cit = column.begin(); cit != column.end(); ++cit) {
                    int64_t timestamp = cit->first;
                    const std::string& value = cit->second;
                    printf("%s %s %ld %s\n",
                            row_key.c_str(), column_key.c_str(), timestamp, value.c_str());
                }
            }
            last_key = row_key+'\0';
        }
    }
    // Delete
    table->Delete("com.baidu.www/");
    // Close
    delete table;
    return 0;
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
