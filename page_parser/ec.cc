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
#include <algorithm>
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
    Table* tag_table = teraeasy::OpenTable("TagTable");
    // Insert
    Record record;
    //table->Write("com.baidu.www/", record);
    // Read
    //table->Read("com.baidu.www/", &record);
    // Modify
    //record["anchor:www.hao123.com/"][time(NULL)] = "°Ù¶È";
    //table->Write("com.baidu.www/", record);
    // Scan
    std::string last_key = (argc > 1) ? argv[1] : "";
    std::string end_key = (argc > 2) ? argv[2] : "";
    while(last_key != "" || last_key <= end_key) {
        TableSlice slice;
        table->Scan(last_key, "~", &slice);
        if (slice.empty()) {
            break;
        }
        for (TableSlice::iterator it = slice.begin(); it != slice.end(); ++it) {
            const Key& row_key = it->first;
            last_key = row_key+'\0';
            Record& record= it->second;
            printf("Row key is %s\n", row_key.c_str());
            if (row_key.find("2014") == std::string::npos) {
                continue;
            }
            Column& page = record["page"];
            //Column& title_c = record["title"];
            if (!page.empty()) {
                std::string& html = page.begin()->second;
                std::string code = "content=\"text/html; charset=utf-8\" />";
                size_t pos = html.find(code);
                if (pos != std::string::npos) {
                    html.replace(pos, code.size(), "");
                }

                int64_t ts = page.begin()->first;
                std::vector<std::string> kws;
                std::sort(kws.begin(), kws.end());
                std::unique(kws.begin(), kws.end());
                std::string title;// = title_c.begin()->second;
                pk::parse_keyword(html, kws);
                pk::parse_title(html, title);
                //kws.push_back("haha");
                //printf("Page: \%s\n", html.c_str());
                for (size_t i=0; i < kws.size(); i++) {
                    int64_t nt = get_micros();
                    std::ostringstream oss;
                    oss << kws[i] << "-" << ts <<  "-" << nt;
                    std::string tag_key =oss.str();
                    Record x;
                    x["url"][0] = row_key;
                    x["title"][0] = title;
                    tag_table->Write(tag_key, x);
                    record["tag"][i] = kws[i];
                }
                table->Write(row_key, record);
            }
        }
    }
    // Close
    delete table;
    delete tag_table;
    return 0;
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
