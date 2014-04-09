/***************************************************************************
 * 
 * Copyright (c) 2014, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 /**
 * @file re_engine.h
 * @author yanshiguang02@baidu.com
 * @date 2014/03/29 13:30:01
 * @brief 
 *  
 **/

#ifndef  ARET_RE_ENGIN_H_
#define  ARET_RE_ENGIN_H_

#include <pthread.h>
#include <string>
#include <vector>
#include <sstream>

namespace teraeasy {
class Table;
}

namespace aret {

class Mutex {
public:
    Mutex() {
        pthread_mutex_init(&_mu, NULL);
    }
    ~Mutex() {
        pthread_mutex_destroy(&_mu);
    }
    void Lock() {
        pthread_mutex_lock(&_mu);
    }
    void UnLock() {
        pthread_mutex_unlock(&_mu);
    }
    friend class MutexLock;
private:
    pthread_mutex_t _mu;
};
class MutexLock {
public:
    MutexLock(Mutex* mu) : _mu(mu) {
        _mu->Lock();
    }
    ~MutexLock() {
        _mu->UnLock();
    }
private:
    Mutex* _mu;
};

enum ActionType {
    AT_VIEW = 0,    ///< 浏览
    AT_IGNORE,      ///< 推荐后,用户没有浏览
    AT_LIKE,        ///< 明确表达喜欢
    AT_DISLIKE,     ///< 明确表示不喜欢
    AT_ARRIVE,      ///< 到达某个地方, 记录用户地理位置
};

class ReEngine {
public:
    ReEngine();
    ~ReEngine();
    bool Init(const std::string& webtable = "WebTable",
              const std::string& tagtable = "TagTable",
              const std::string& usertable = "WebUserTable") { return true;};
    /**
     * @brief 获取推荐结果
     * @param [in] user: 用户id
     * @param [in] num: 需要的数量
     * @param [out] urls: 推荐系统给出的结果
     * @param [out] urls: 推荐系统给出的结果描述
    **/
    bool GetRecommend(const std::string& user, int num,
                      std::vector<std::string>* urls,
                      std::vector<std::string>* desc,
                      std::vector<std::string>* tags = NULL,
                      std::string* err = NULL);
    /**
     * @brief 记录一次用户行为
     * @param [in] user: 用户id
     * @param [in] action: 行为类型
     * @param [in] content: 行为描述, 一个url或者一个地点
    **/
    bool RecordActions(const std::string& user, ActionType action, 
                       const std::vector<std::string>& content,
                       std::string* err = NULL);
    /// 获取html
    bool GetHtml(const std::string& url, std::string* html);
private:
    int GetUrlForTag(const std::string tag, int num, std::vector<std::string>* urls,
                     std::vector<std::string>* titles, std::ostringstream& err);
    bool RandomRecommend(int num, std::vector<std::string>* urls,
                         std::vector<std::string>* desc,
                         std::vector<std::string>* tags,
                         std::string* err);
private:
    Mutex               _mu;
    teraeasy::Table*    _webtable;
    teraeasy::Table*    _tagtable;
    teraeasy::Table*    _usertable;
};

}
#endif  //__RE_ENGIN_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
