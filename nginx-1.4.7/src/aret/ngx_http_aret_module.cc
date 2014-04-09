/**
 * @file ngx_aret_module.c
 * @author yanshiguang02@baidu.com
 * @date 2014/03/27 08:32:27
 * @brief 
 *  
 **/

extern "C" {

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

#include "re_engine.h"

#include "tera_easy.h"

aret::ReEngine g_re_engine;

using namespace teraeasy;
/// 推荐的参数
typedef struct {
    ngx_str_t user_id;
    ngx_str_t url;
} ngx_http_aret_loc_conf_t;

/// 推荐的前向声明
static char *ngx_http_recommend_readconf(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_view_readconf(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

/// 配置操作声明
static void *ngx_http_aret_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_aret_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);

/// 指令序列
static ngx_command_t ngx_http_aret_commands[] = {
    { ngx_string("recommend"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_http_recommend_readconf,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_aret_loc_conf_t, user_id),
        NULL },
    { ngx_string("view"),
        NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_http_view_readconf,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_aret_loc_conf_t, url),
        NULL },
    ngx_null_command
};

/// http context
static ngx_http_module_t ngx_http_aret_module_ctx = {
    NULL, /* preconfiguration */
    NULL, /* postconfiguration */
    NULL, /* create main configuration */
    NULL, /* init main configuration */
    NULL, /* create server configuration */
    NULL, /* merge server configuration */
    ngx_http_aret_create_loc_conf, /* create location configration */
    ngx_http_aret_merge_loc_conf /* merge location configration */
};

/// 模块定义
ngx_module_t ngx_http_aret_module = {
    NGX_MODULE_V1,
    &ngx_http_aret_module_ctx, /* module context */
    ngx_http_aret_commands, /* module directives */
    NGX_HTTP_MODULE, /* module type */
    NULL, /* init master */
    NULL, /* init module */
    NULL, /* init process */
    NULL, /* init thread */
    NULL, /* exit thread */
    NULL, /* exit process */
    NULL, /* exit master */
    NGX_MODULE_V1_PADDING
};

static char* str_to_buf(ngx_pool_t* pool, const std::string& str)
{
    char* ret_buf = (char*) ngx_pcalloc(pool, str.size());
    memcpy(ret_buf, str.data(), str.size());
    return ret_buf;
}
/// Handler
static ngx_int_t ngx_http_recommend_handler(ngx_http_request_t *r)
{
    ngx_int_t rc;
    ngx_buf_t *b;
    ngx_chain_t out;
    ngx_http_aret_loc_conf_t *elcf;
    elcf = (ngx_http_aret_loc_conf_t*) ngx_http_get_module_loc_conf(r, ngx_http_aret_module);
    if(!(r->method & (NGX_HTTP_HEAD|NGX_HTTP_GET|NGX_HTTP_POST)))
    {
        return NGX_HTTP_NOT_ALLOWED;
    }

    // 从TagTable选一段作为推荐内容
    std::vector<std::string> urls;
    std::vector<std::string> titles;
    std::vector<std::string> tags;
    std::string err;
    std::string content;
    if (g_re_engine.GetRecommend("user", 10, &urls, &titles, &tags, &err)) {
        for (size_t i=0; i<urls.size(); i++)
        {
            content.append("<a href=view?url=");
            content.append(urls[i]);
            content.append(">");
            content.append(titles[i]);
            content.append("</a><br/>");
            content.append("Tag: ");
            content.append(tags[i]);
            content.append("<br/>");
        }
    } else {
        content = "GetRecommend fail~";
    }
    char* send_buf = str_to_buf(r->pool, err + content);

    r->headers_out.content_type.len = sizeof("text/html") - 1;
    r->headers_out.content_type.data = (u_char *) "text/html";
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = content.size();
    if(r->method == NGX_HTTP_HEAD)
    {
        rc = ngx_http_send_header(r);
        if(rc != NGX_OK)
        {
            return rc;
        }
    }
    b = (ngx_buf_t*)ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    if(b == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Failed to allocate response buffer.");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    out.buf = b;
    out.next = NULL;
    b->pos = (u_char*)send_buf;//elcf->user_id.data;
    b->last = b->pos + content.size();//elcf->user_id.data + (elcf->user_id.len);
    b->memory = 1;
    b->last_buf = 1;
    rc = ngx_http_send_header(r);
    if(rc != NGX_OK)
    {
        return rc;
    }
    return ngx_http_output_filter(r, &out);
}

static bool parse_args(const char* args, std::string* user, std::string* url) {
    char userbuf[128];
    char urlbuf[1024];
    if (2 == sscanf(args, "user=%s&url=%s", userbuf, urlbuf))
    {
        user->assign(userbuf);
        url->assign(urlbuf);
    }
    else if (1 == sscanf(args, "url=%s", urlbuf))
    {
        url->assign(urlbuf);
    }
    else
    {
        return false;
    }
    return true;
}

/// View Handler
static ngx_int_t ngx_http_view_handler(ngx_http_request_t *r)
{
    ngx_int_t rc;
    ngx_buf_t *b;
    ngx_chain_t out;
    ngx_http_aret_loc_conf_t *elcf;
    elcf = (ngx_http_aret_loc_conf_t*)ngx_http_get_module_loc_conf(r, ngx_http_aret_module);
    if(!(r->method & (NGX_HTTP_HEAD|NGX_HTTP_GET|NGX_HTTP_POST)))
    {
        return NGX_HTTP_NOT_ALLOWED;
    }
    std::string url;
    std::string user;
    parse_args(reinterpret_cast<const char*>(r->args.data), &user, &url);
    std::vector<std::string> content;
    content.push_back(url);
    std::string err;
    g_re_engine.RecordActions("user", aret::AT_VIEW, content, &err);
    std::string html;
    if (url == "" || !g_re_engine.GetHtml(url, &html))
    {
        html = "No this page in WebTable";
    }
    char* send_buf = str_to_buf(r->pool, err + html);
    r->headers_out.content_type.len = sizeof("text/html") - 1;
    r->headers_out.content_type.data = (u_char *) "text/html";
    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = html.size();
    if(r->method == NGX_HTTP_HEAD)
    {
        rc = ngx_http_send_header(r);
        if(rc != NGX_OK)
        {
            return rc;
        }
    }
    b = (ngx_buf_t*)ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    if(b == NULL)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Failed to allocate response buffer.");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    out.buf = b;
    out.next = NULL;
    b->pos = (u_char*)send_buf;
    b->last = (u_char*)send_buf + html.size();
    b->memory = 1;
    b->last_buf = 1;
    rc = ngx_http_send_header(r);
    if(rc != NGX_OK)
    {
        return rc;
    }
    return ngx_http_output_filter(r, &out);
}

/// 推荐文章 readconf
static char * ngx_http_recommend_readconf(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf;
    clcf = (ngx_http_core_loc_conf_t*)ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_recommend_handler;
    ngx_conf_set_str_slot(cf,cmd,conf);
    return NGX_CONF_OK;
}
/// 显示文章 readconf
static char * ngx_http_view_readconf(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t *clcf;
    clcf = (ngx_http_core_loc_conf_t*)ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_view_handler;
    ngx_conf_set_str_slot(cf,cmd,conf);
    return NGX_CONF_OK;
}

static void * ngx_http_aret_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_aret_loc_conf_t *conf;
    conf = (ngx_http_aret_loc_conf_t*)ngx_pcalloc(cf->pool, sizeof(ngx_http_aret_loc_conf_t));
    if (conf == NULL) {
        return NGX_CONF_ERROR;
    }
    conf->user_id.len = 0;
    conf->user_id.data = NULL;
    conf->url.len = 0;
    conf->url.data = NULL;
    return conf;
}

///
static char *ngx_http_aret_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_aret_loc_conf_t *prev = (ngx_http_aret_loc_conf_t*)parent;
    ngx_http_aret_loc_conf_t *conf = (ngx_http_aret_loc_conf_t*)child;
    ngx_conf_merge_str_value(conf->user_id, prev->user_id, "");
    ngx_conf_merge_str_value(conf->user_id, prev->url, "");
    return NGX_CONF_OK;
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
