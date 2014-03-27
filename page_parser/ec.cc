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

#include <sstream>
#include <vector>
#include <sys/time.h>

#include "pk.h"
#include "tera_easy.h"

using teraeasy::Table;
using teraeasy::OpenTable;
using teraeasy::TableSlice;
using teraeasy::Record;
using teraeasy::Column;
using teraeasy::Key;

static inline long get_micros() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return static_cast<long>(tv.tv_sec) * 1000000 + tv.tv_usec;
}
int main(int argc, char* argv[]) {
    // Open
    Table* table = teraeasy::OpenTable("WebTable");
    Table* tag_table = teraeasy::OpenTable("TestTagTable");
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
        for (TableSlice::iterator it = slice.begin(); it != slice.end(); ++it) {
            const Key& row_key = it->first;
            Record& record= it->second;
            printf("Row key is %s\n", row_key.c_str());
            Column& page = record["page"];
            if (!page.empty()) {
                std::string& html = page.begin()->second;
                int64_t ts = page.begin()->first;
                std::vector<std::string> kws = pk::parse(html);
                //kws.push_back("haha");
                //printf("Page: \%s\n", html.c_str());
                for (size_t i=0; i < kws.size(); i++) {
                    int64_t nt = get_micros();
                    std::ostringstream oss;
                    oss << kws[i] << "-" << ts <<  "-" << nt;
                    std::string tag_key =oss.str();
                    Record x;
                    x["url"][0] = row_key;
                    tag_table->Write(tag_key, x);
                }
            }
            last_key = row_key+'\0';
        }
    }
    // Close
    delete table;
    delete tag_table;
    return 0;
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
