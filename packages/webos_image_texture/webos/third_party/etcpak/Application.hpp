#ifndef __APPLICATION_HPP__
#define __APPLICATION_HPP__
#include <string>
bool etcpak_encoder(const std::string& url, bool hasAlpha,const char* input,const char* output);
void etcpak_decoder(const char* input,const char* output);
#endif
