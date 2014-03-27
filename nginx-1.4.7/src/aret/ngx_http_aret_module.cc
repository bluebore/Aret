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

#include "tera_easy.h"

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
    static std::string last_key = "";
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
    Table* tag_table =  OpenTable("TestTagTable");
    TableSlice slice;
    tag_table->Scan(last_key, "~", &slice);
    std::string content;
    int num = 0;
    std::string key;
    for (TableSlice::iterator it = slice.begin(); it != slice.end(); ++it)
    {
        Record& record= it->second;
        const std::string url = record["url"].begin()->second;
        const std::string title = record["title"].begin()->second;
        content.append("<a href=view?url=");
        content.append(url);
        content.append(">");
        content.append(title);
        content.append("</a><br/>");
        if (++num > 10)
            break;
        key = it->first;
    }
    last_key = key;
    content.push_back('\0');
    delete tag_table;
    char* send_buf = str_to_buf(r->pool, content);

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

/// 根据url从网页库获取html
bool get_html(const std::string& url, std::string *html) {
    Table* table = teraeasy::OpenTable("WebTable");
    Record record;
    if (table->Read(url, &record) && !record.empty())
    {
        Column& page = record["page"];
        if (!page.empty())
        {
            std::string& htmlstr = page.begin()->second;
            html->assign(htmlstr);
        }
    }
    delete table;
    return !html->empty();
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
    if ((r->args.len>4) && strncmp("url=", (char*)r->args.data, 4) == 0) {
        url.assign((char*)r->args.data + 4, r->args.len - 4);
    }
    std::string html;
    if (url == "" || !get_html(url, &html))
    {
        html = "No this page in WebTable";
    }
    char* send_buf = str_to_buf(r->pool, html);
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
