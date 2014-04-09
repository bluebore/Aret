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
    AT_VIEW = 0,    ///< ���
    AT_IGNORE,      ///< �Ƽ���,�û�û�����
    AT_LIKE,        ///< ��ȷ���ϲ��
    AT_DISLIKE,     ///< ��ȷ��ʾ��ϲ��
    AT_ARRIVE,      ///< ����ĳ���ط�, ��¼�û�����λ��
};

class ReEngine {
public:
    ReEngine();
    ~ReEngine();
    bool Init(const std::string& webtable = "WebTable",
              const std::string& tagtable = "TagTable",
              const std::string& usertable = "WebUserTable") { return true;};
    /**
     * @brief ��ȡ�Ƽ����
     * @param [in] user: �û�id
     * @param [in] num: ��Ҫ������
     * @param [out] urls: �Ƽ�ϵͳ�����Ľ��
     * @param [out] urls: �Ƽ�ϵͳ�����Ľ������
    **/
    bool GetRecommend(const std::string& user, int num,
                      std::vector<std::string>* urls,
                      std::vector<std::string>* desc,
                      std::vector<std::string>* tags = NULL,
                      std::string* err = NULL);
    /**
     * @brief ��¼һ���û���Ϊ
     * @param [in] user: �û�id
     * @param [in] action: ��Ϊ����
     * @param [in] content: ��Ϊ����, һ��url����һ���ص�
    **/
    bool RecordActions(const std::string& user, ActionType action, 
                       const std::vector<std::string>& content,
                       std::string* err = NULL);
    /// ��ȡhtml
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
