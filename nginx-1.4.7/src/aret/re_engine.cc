/***************************************************************************
 * 
 * Copyright (c) 2014, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 /**
 * @file re_engine.cc
 * @author yanshiguang02@baidu.com
 * @date 2014/03/29 13:30:20
 * @brief 
 *  
 **/

#include "re_engine.h"

#include <pthread.h>
#include <assert.h>
#include <sstream>
#include "tera_easy.h"

using teraeasy::Table;
using teraeasy::Record;
using teraeasy::Column;
using teraeasy::TableSlice;

namespace aret {

ReEngine::ReEngine() {
    _webtable = teraeasy::OpenTable("WebTable");
    _tagtable = teraeasy::OpenTable("TagTable");
    _usertable = teraeasy::OpenTable("WebUserTable");
    if(!_webtable || !_usertable || !_tagtable) {
        delete _webtable;
        delete _usertable;
        delete _tagtable;
        throw "Open Tera table fail~";
    }
}

ReEngine::~ReEngine() {
    delete _webtable;
    _webtable = NULL;
    delete _tagtable;
    _tagtable = NULL;
    delete _usertable;
    _usertable = NULL;
}

bool ReEngine::GetHtml(const std::string& url, std::string* html) {
    Record record;
    if (_webtable->Read(url, &record) && !record.empty())
    {
        Column& page = record["page"];
        if (!page.empty())
        {
            std::string& htmlstr = page.begin()->second;
            html->assign(htmlstr);
        }
    }
    return !html->empty();
}
int ReEngine::GetUrlForTag(const std::string tag, int num, std::vector<std::string>* urls,
                           std::vector<std::string>* titles,
                           std::ostringstream& err) {
    TableSlice slice;
    if (!_tagtable->Scan(tag, "~", &slice) || slice.empty()) {
        err << "Scan tagtable " << tag << "fail <br/>" << std::endl;
        return 0;
    }
    int ret = 0;
    for(TableSlice::iterator it = slice.begin();
        it != slice.end() && ret < num; ++it) {
        const std::string& row_key = it->first;
        if (row_key.find(tag) == std::string::npos) {
            err << "row_key " << row_key << " find " << tag << "fail <br/>" << std::endl;
            return ret;
        }
        Record& row = it->second;
        if (row.empty()) {
            err << "Empty row in user table<br/>";
            continue;
        }
        urls->push_back(row["url"][0]);
        titles->push_back(row["title"][0]);
        ret ++;
    }
    return ret;
}

bool ReEngine::RandomRecommend(int num, std::vector<std::string>* urls,
                               std::vector<std::string>* desc,
                               std::vector<std::string>* tags,
                               std::string* err) {
    static std::string last_rand = "";
    TableSlice slice;
    if (!_tagtable->Scan(last_rand, "~", &slice)) {
        last_rand = "";
        _tagtable->Scan(last_rand, "~", &slice);
    }
    int n = 0;
    for (TableSlice::iterator it = slice.begin(); it != slice.end() && n<num; ++it)
    {
        const std::string& row_key = it->first;
        Record& record= it->second;
        const std::string& url = record["url"].begin()->second;
        const std::string& title = record["title"].begin()->second;
        urls->push_back(url);
        desc->push_back(title);
        tags->push_back(row_key);
        last_rand = row_key;
        last_rand.push_back('\0');
        n++;
    }
    return true;
}

struct TagItem {
    int         num;
    std::string tag;
    bool operator<(const TagItem& t) const {
        return num > t.num;
    }
};
bool ReEngine::GetRecommend(const std::string& user, int num,
                            std::vector<std::string>* urls,
                            std::vector<std::string>* desc,
                            std::vector<std::string>* tags,
                            std::string* err) {
    MutexLock lock(&_mu);
    const double recommend_radio = 0.7;
    std::ostringstream serr;
    // 获取用户喜好推荐
    Record user_record;
    if (_usertable->Read(user, &user_record) && user_record.size()) {
        Record::iterator rcit = user_record.lower_bound("tag");
        std::vector<double> prios;
        std::vector<TagItem> tag_items;
        while(rcit != user_record.end() && rcit->first < "tag~")  {
            const std::string& ckey = rcit->first;
            size_t pos = ckey.find(":");
            TagItem tt;
            tt.tag = ckey.substr(pos+1);
            tt.num = atoi((rcit->second)[0].c_str());
            tag_items.push_back(tt);
            ++rcit;
        }
        sort(tag_items.begin(), tag_items.end());
        double all_prio = 0.0;
        for (size_t i=0;i<tag_items.size() && i< 5; i++) {
            all_prio += tag_items[i].num;
            prios.push_back(tag_items[i].num);
        }
        if (all_prio == 0) {
            *err = "No url can be Recommend: all_prio == 0";
            return false;
        }
        double di = 1 / all_prio;
        serr << "di= " << di << "<br/>" << std::endl;
        for (size_t i=0; i<tag_items.size() && i < 5; i++) {
            int tag_num = static_cast<int>(prios[i] * di * num * recommend_radio);
            if (tag_num > 2) tag_num = 2;
            int n = GetUrlForTag(tag_items[i].tag, tag_num, urls, desc, serr);
            serr << "GetUrlForTag " << prios[i] << " \"" << tag_items[i].tag << "\" " 
                 << tag_num << " ret: " << n << "<br/>" << std::endl;
            for(int j=0; j<n; j++)
                tags->push_back(tag_items[i].tag);
        }
    }

    // 随机补足
    bool ret = RandomRecommend(num - urls->size(), urls, desc, tags, err);
    for (size_t i=0; i<tags->size(); i++) {
        size_t pos = (*tags)[i].rfind("-");
        if (pos != std::string::npos) {
            pos = (*tags)[i].rfind("-", pos-1);
            if (pos != std::string::npos) {
                (*tags)[i].resize(pos);
            }
        }
    }
    serr << "url_num: " << urls->size() << ", title_num: " << desc->size()
         << "<br/>" << std::endl;
    err->assign(serr.str());
    return ret;
}

bool ReEngine::RecordActions(const std::string& user, ActionType action, 
                             const std::vector<std::string>& content,
                             std::string* err) {
    if (action != AT_VIEW) {
        return true;
    }
    std::stringstream serr;
    /// 查出 view 包含了哪些tag
    std::vector<std::string> tags;
    for (size_t i=0; i<content.size();i++) {
        Record url_record;
        _webtable->Read(content[i], &url_record);
        Column& url_column = url_record["tag"];
        for (Column::iterator it = url_column.begin(); it != url_column.end(); ++it) {
            tags.push_back(it->second);
        }
    }
    serr << "tags num: " << tags.size() << std::endl;
    /// 记录这些tag的view
    Record user_record;
    _usertable->Read(user, &user_record);
    for (size_t i=0; i<tags.size(); i++) {
        std::string now_tag = "tag:" + tags[i];
        Column& tag_column = user_record[now_tag];
        if (tag_column.empty()) {
            tag_column[0] = "1";
        } else {
            int num = atoi(tag_column[0].c_str());
            std::ostringstream oss;
            oss << num+1;
            tag_column[0] = oss.str();
        }
    }
    _usertable->Write(user, user_record);
    err->assign(serr.str());
    return true;
}

}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
