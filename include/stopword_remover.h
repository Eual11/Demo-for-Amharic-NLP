#ifndef _STOP_WORD_REMOVER_H
#define _STOP_WORD_REMOVER_H
#include <codecvt>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <regex>
#include <string>

std::wstring stopword_remove(const std::wstring &string);

#endif
