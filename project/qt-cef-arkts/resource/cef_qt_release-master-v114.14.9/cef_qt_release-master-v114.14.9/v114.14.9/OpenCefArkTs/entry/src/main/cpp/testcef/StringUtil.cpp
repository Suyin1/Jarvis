// Copyright (c) 2022 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.+
// 字符串工具函数 — CEF CString 与 std::string 相互转换辅助

#include "StringUtil.h"
#include <algorithm>

namespace client {

    std::string AsciiStrToLower(const std::string &str) {
        std::string lowerStr = str;
        std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
        return lowerStr;
    }

    std::string AsciiStrReplace(const std::string &str, const std::string &from, const std::string &to) {
        std::string result = str;
        std::string::size_type pos = 0;
        std::string::size_type from_len = from.length();
        std::string::size_type to_len = to.length();
        do {
            pos = result.find(from, pos);
            if (pos != std::string::npos) {
                result.replace(pos, from_len, to);
                pos += to_len;
            }
        } while (pos != std::string::npos);
        return result;
    }

} // namespace client
