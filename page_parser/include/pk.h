/***************************************************************************
 * 
 * Copyright (c) 2014 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
 
 
/**
 * @file parse_keyword.h
 * @author shichengyi(com@baidu.com)
 * @date 2014/03/27 00:56:43
 * @brief 
 *  
 **/

#ifndef  __PARSE_KEYWORD_H_
#define  __PARSE_KEYWORD_H_


namespace pk
{

void parse_keyword(const std::string& html, std::vector<std::string>& vs);

void parse_title(const std::string& html, std::string& title);

}

#endif  //__PARSE_KEYWORD_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
