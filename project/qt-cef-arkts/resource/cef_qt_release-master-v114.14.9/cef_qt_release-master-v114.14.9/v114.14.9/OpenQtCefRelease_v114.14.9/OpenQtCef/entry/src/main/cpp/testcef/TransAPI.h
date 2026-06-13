// Copyright (c) 2023 Huawei Device Co., Ltd. All rights reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef CEF_TESTS_CEFSIMPLE_SIMPLE_DEMO_TRANS_API_H_
#define CEF_TESTS_CEFSIMPLE_SIMPLE_DEMO_TRANS_API_H_

#include <iomanip>
#include <iostream>
#include <memory>
#include <string>

#include "include/base/cef_callback.h"
#include "include/cef_parser.h"
#include "include/wrapper/cef_helpers.h"
#include "include/wrapper/cef_stream_resource_handler.h"
#include "StringUtil.h"

namespace {
    const char kTestHost[] = "tests";
    const char kLocalHost[] = "localhost";
    bool IsTestURL(const std::string &url, const std::string &path) { return url.find(path) != std::string::npos; }

    std::string DumpRequestContents(CefRefPtr<CefRequest> request) {
        std::stringstream ss;

        ss << "URL: " << std::string(request->GetURL());
        ss << "\nMethod: " << std::string(request->GetMethod());

        CefRequest::HeaderMap headerMap;
        request->GetHeaderMap(headerMap);
        if (headerMap.size() > 0) {
            ss << "\nHeaders:";
            CefRequest::HeaderMap::const_iterator it = headerMap.begin();
            for (; it != headerMap.end(); ++it) {
                ss << "\n\t" << std::string((*it).first) << ": " << std::string((*it).second);
            }
        }

        CefRefPtr<CefPostData> postData = request->GetPostData();
        if (postData.get()) {
            CefPostData::ElementVector elements;
            postData->GetElements(elements);
            if (elements.size() > 0) {
                ss << "\nPost Data:";
                CefRefPtr<CefPostDataElement> element;
                CefPostData::ElementVector::const_iterator it = elements.begin();
                for (; it != elements.end(); ++it) {
                    element = (*it);
                    if (element->GetType() == PDE_TYPE_BYTES) {
                        // the element is composed of bytes
                        ss << "\n\tBytes: ";
                        if (element->GetBytesCount() == 0) {
                            ss << "(empty)";
                        } else {
                            // retrieve the data.
                            size_t size = element->GetBytesCount();
                            char *bytes = new char[size];
                            element->GetBytes(size, bytes);
                            ss << std::string(bytes, size);
                            delete[] bytes;
                        }
                    } else if (element->GetType() == PDE_TYPE_FILE) {
                        ss << "\n\tFile: " << std::string(element->GetFile());
                    }
                }
            }
        }

        return ss.str();
    }

    CefRefPtr<CefStreamReader> GetDumpResponse(CefRefPtr<CefRequest> request,
                                               CefResponse::HeaderMap &response_headers) {
        std::string origin;

        // Extract the origin request header, if any. It will be specified for
        // cross-origin requests.
        {
            CefRequest::HeaderMap requestMap;
            request->GetHeaderMap(requestMap);

            CefRequest::HeaderMap::const_iterator it = requestMap.begin();
            for (; it != requestMap.end(); ++it) {
                const std::string &key = client::AsciiStrToLower(it->first);
                if (key == "origin") {
                    origin = it->second;
                    break;
                }
            }
        }

        if (!origin.empty() && (origin.find("http://" + std::string(kTestHost)) == 0 ||
                                origin.find("http://" + std::string(kLocalHost)) == 0)) {
            // Allow cross-origin XMLHttpRequests from test origins.
            response_headers.insert(std::make_pair("Access-Control-Allow-Origin", origin));

            // Allow the custom header from the xmlhttprequest.html example.
            response_headers.insert(std::make_pair("Access-Control-Allow-Headers", "My-Custom-Header"));
        }

        const std::string &dump = DumpRequestContents(request);
        std::string str = "<html><body bgcolor=\"white\"><pre>" + dump + "</pre></body></html>";
        CefRefPtr<CefStreamReader> stream =
            CefStreamReader::CreateForData(static_cast<void *>(const_cast<char *>(str.c_str())), str.size());
        DCHECK(stream);
        return stream;
    }

    // Returns a data: URI with the specified contents.
    std::string GetDataURI(const std::string &data, const std::string &mime_type) {
        return "data:" + mime_type + ";base64," +
               CefURIEncode(CefBase64Encode(data.data(), data.size()), false).ToString();
    }

    std::string GetErrorString(cef_errorcode_t code) {
// Case condition that returns |code| as a string.
#define CASE(code)                                                                                                     \
    case code:                                                                                                         \
        return #code

        switch (code) {
            CASE(ERR_NONE);
            CASE(ERR_FAILED);
            CASE(ERR_ABORTED);
            CASE(ERR_INVALID_ARGUMENT);
            CASE(ERR_INVALID_HANDLE);
            CASE(ERR_FILE_NOT_FOUND);
            CASE(ERR_TIMED_OUT);
            CASE(ERR_FILE_TOO_BIG);
            CASE(ERR_UNEXPECTED);
            CASE(ERR_ACCESS_DENIED);
            CASE(ERR_NOT_IMPLEMENTED);
            CASE(ERR_CONNECTION_CLOSED);
            CASE(ERR_CONNECTION_RESET);
            CASE(ERR_CONNECTION_REFUSED);
            CASE(ERR_CONNECTION_ABORTED);
            CASE(ERR_CONNECTION_FAILED);
            CASE(ERR_NAME_NOT_RESOLVED);
            CASE(ERR_INTERNET_DISCONNECTED);
            CASE(ERR_SSL_PROTOCOL_ERROR);
            CASE(ERR_ADDRESS_INVALID);
            CASE(ERR_ADDRESS_UNREACHABLE);
            CASE(ERR_SSL_CLIENT_AUTH_CERT_NEEDED);
            CASE(ERR_TUNNEL_CONNECTION_FAILED);
            CASE(ERR_NO_SSL_VERSIONS_ENABLED);
            CASE(ERR_SSL_VERSION_OR_CIPHER_MISMATCH);
            CASE(ERR_SSL_RENEGOTIATION_REQUESTED);
            CASE(ERR_CERT_COMMON_NAME_INVALID);
            CASE(ERR_CERT_DATE_INVALID);
            CASE(ERR_CERT_AUTHORITY_INVALID);
            CASE(ERR_CERT_CONTAINS_ERRORS);
            CASE(ERR_CERT_NO_REVOCATION_MECHANISM);
            CASE(ERR_CERT_UNABLE_TO_CHECK_REVOCATION);
            CASE(ERR_CERT_REVOKED);
            CASE(ERR_CERT_INVALID);
            CASE(ERR_CERT_END);
            CASE(ERR_INVALID_URL);
            CASE(ERR_DISALLOWED_URL_SCHEME);
            CASE(ERR_UNKNOWN_URL_SCHEME);
            CASE(ERR_TOO_MANY_REDIRECTS);
            CASE(ERR_UNSAFE_REDIRECT);
            CASE(ERR_UNSAFE_PORT);
            CASE(ERR_INVALID_RESPONSE);
            CASE(ERR_INVALID_CHUNKED_ENCODING);
            CASE(ERR_METHOD_NOT_SUPPORTED);
            CASE(ERR_UNEXPECTED_PROXY_AUTH);
            CASE(ERR_EMPTY_RESPONSE);
            CASE(ERR_RESPONSE_HEADERS_TOO_BIG);
            CASE(ERR_CACHE_MISS);
            CASE(ERR_INSECURE_RESPONSE);
        default:
            return "UNKNOWN";
        }
    }

    std::string GetTimeString(const CefTime &value) {
        if (value.GetTimeT() == 0) {
            return "Unspecified";
        }

        static const char *kMonths[] = {"January", "February", "March",     "April",   "May",      "June",
                                        "July",    "August",   "September", "October", "November", "December"};
        std::string month;
        if (value.month >= 1 && value.month <= 12) {
            month = kMonths[value.month - 1];
        } else {
            month = "Invalid";
        }

        std::stringstream ss;
        ss << month << " " << value.day_of_month << ", " << value.year << " " << std::setfill('0') << std::setw(2)
           << value.hour << ":" << std::setfill('0') << std::setw(2) << value.minute << ":" << std::setfill('0')
           << std::setw(2) << value.second;
        return ss.str();
    }

    std::string GetTimeString(const CefBaseTime &value) {
        CefTime time;
        if (cef_time_from_basetime(value, &time)) {
            return GetTimeString(time);
        } else {
            return "Invalid";
        }
    }

    std::string GetBinaryString(CefRefPtr<CefBinaryValue> value) {
        if (!value.get()) {
            return "&nbsp;";
        }

        // Retrieve the value.
        const size_t size = value->GetSize();
        std::string src;
        src.resize(size);
        value->GetData(const_cast<char *>(src.data()), size, 0);

        // Encode the value.
        return CefBase64Encode(src.data(), src.size());
    }

#define FLAG(flag)                                                                                                     \
    if (status & flag) {                                                                                               \
        result += std::string(#flag) + "<br/>";                                                                        \
    }

#define VALUE(val, def)                                                                                                \
    if (val == def) {                                                                                                  \
        return std::string(#def);                                                                                      \
    }

    std::string GetCertStatusString(cef_cert_status_t status) {
        std::string result;

        FLAG(CERT_STATUS_COMMON_NAME_INVALID);
        FLAG(CERT_STATUS_DATE_INVALID);
        FLAG(CERT_STATUS_AUTHORITY_INVALID);
        FLAG(CERT_STATUS_NO_REVOCATION_MECHANISM);
        FLAG(CERT_STATUS_UNABLE_TO_CHECK_REVOCATION);
        FLAG(CERT_STATUS_REVOKED);
        FLAG(CERT_STATUS_INVALID);
        FLAG(CERT_STATUS_WEAK_SIGNATURE_ALGORITHM);
        FLAG(CERT_STATUS_NON_UNIQUE_NAME);
        FLAG(CERT_STATUS_WEAK_KEY);
        FLAG(CERT_STATUS_PINNED_KEY_MISSING);
        FLAG(CERT_STATUS_NAME_CONSTRAINT_VIOLATION);
        FLAG(CERT_STATUS_VALIDITY_TOO_LONG);
        FLAG(CERT_STATUS_IS_EV);
        FLAG(CERT_STATUS_REV_CHECKING_ENABLED);
        FLAG(CERT_STATUS_SHA1_SIGNATURE_PRESENT);
        FLAG(CERT_STATUS_CT_COMPLIANCE_FAILED);

        if (result.empty()) {
            return "&nbsp;";
        }
        return result;
    }

    std::string GetSSLVersionString(cef_ssl_version_t version) {
        VALUE(version, SSL_CONNECTION_VERSION_UNKNOWN);
        VALUE(version, SSL_CONNECTION_VERSION_SSL2);
        VALUE(version, SSL_CONNECTION_VERSION_SSL3);
        VALUE(version, SSL_CONNECTION_VERSION_TLS1);
        VALUE(version, SSL_CONNECTION_VERSION_TLS1_1);
        VALUE(version, SSL_CONNECTION_VERSION_TLS1_2);
        VALUE(version, SSL_CONNECTION_VERSION_TLS1_3);
        VALUE(version, SSL_CONNECTION_VERSION_QUIC);
        return std::string();
    }

    std::string GetContentStatusString(cef_ssl_content_status_t status) {
        std::string result;

        VALUE(status, SSL_CONTENT_NORMAL_CONTENT);
        FLAG(SSL_CONTENT_DISPLAYED_INSECURE_CONTENT);
        FLAG(SSL_CONTENT_RAN_INSECURE_CONTENT);

        if (result.empty()) {
            return "&nbsp;";
        }
        return result;
    }

    // Load a data: URI containing the error message.
    void LoadErrorPage(CefRefPtr<CefFrame> frame, const std::string &failed_url, cef_errorcode_t error_code,
                       const std::string &other_info) {
        std::stringstream ss;
        ss << "<html><head><title>Page failed to load</title></head>"
              "<body bgcolor=\"white\">"
              "<h3>Page failed to load.</h3>"
              "URL: <a href=\""
           << failed_url << "\">" << failed_url << "</a><br/>Error: " << GetErrorString(error_code) << " ("
           << error_code << ")";

        if (!other_info.empty()) {
            ss << "<br/>" << other_info;
        }

        ss << "</body></html>";
        frame->LoadURL(GetDataURI(ss.str(), "text/html"));
    }

    // Return HTML string with information about a certificate.
    std::string GetCertificateInformation(CefRefPtr<CefX509Certificate> cert, cef_cert_status_t certstatus) {
        CefRefPtr<CefX509CertPrincipal> subject = cert->GetSubject();
        CefRefPtr<CefX509CertPrincipal> issuer = cert->GetIssuer();

        // Build a table showing certificate information. Various types of invalid
        // certificates can be tested using https://badssl.com/.
        std::stringstream ss;
        ss << "<h3>X.509 Certificate Information:</h3>"
              "<table border=1><tr><th>Field</th><th>Value</th></tr>";

        if (certstatus != CERT_STATUS_NONE) {
            ss << "<tr><td>Status</td><td>" << GetCertStatusString(certstatus) << "</td></tr>";
        }

        ss << "<tr><td>Subject</td><td>" << (subject.get() ? subject->GetDisplayName().ToString() : "&nbsp;")
           << "</td></tr>"
              "<tr><td>Issuer</td><td>"
           << (issuer.get() ? issuer->GetDisplayName().ToString() : "&nbsp;")
           << "</td></tr>"
              "<tr><td>Serial #*</td><td>"
           << GetBinaryString(cert->GetSerialNumber()) << "</td></tr>"
           << "<tr><td>Valid Start</td><td>" << GetTimeString(cert->GetValidStart())
           << "</td></tr>"
              "<tr><td>Valid Expiry</td><td>"
           << GetTimeString(cert->GetValidExpiry()) << "</td></tr>";

        CefX509Certificate::IssuerChainBinaryList der_chain_list;
        CefX509Certificate::IssuerChainBinaryList pem_chain_list;
        cert->GetDEREncodedIssuerChain(der_chain_list);
        cert->GetPEMEncodedIssuerChain(pem_chain_list);
        DCHECK_EQ(der_chain_list.size(), pem_chain_list.size());

        der_chain_list.insert(der_chain_list.begin(), cert->GetDEREncoded());
        pem_chain_list.insert(pem_chain_list.begin(), cert->GetPEMEncoded());

        for (size_t i = 0U; i < der_chain_list.size(); ++i) {
            ss << "<tr><td>DER Encoded*</td>"
                  "<td style=\"max-width:800px;overflow:scroll;\">"
               << GetBinaryString(der_chain_list[i])
               << "</td></tr>"
                  "<tr><td>PEM Encoded*</td>"
                  "<td style=\"max-width:800px;overflow:scroll;\">"
               << GetBinaryString(pem_chain_list[i]) << "</td></tr>";
        }

        ss << "</table> * Displayed value is base64 encoded.";
        return ss.str();
    }

} // namespace

#endif // CEF_TESTS_CEFSIMPLE_SIMPLE_DEMO_TRANS_API_H_
