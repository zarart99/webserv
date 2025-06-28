#include "MimeTypes.hpp"

const std::map<std::string, std::string>& getMimeTypes()
{
    static std::map<std::string, std::string> mime;
    if (mime.empty())
    {
        mime[".html"] = "text/html";
        mime[".htm"] = "text/html";
        mime[".css"] = "text/css";
        mime[".js"] = "application/javascript";
        mime[".json"] = "application/json";
        mime[".xml"] = "application/xml";
        mime[".txt"] = "text/plain";
        mime[".png"] = "image/png";
        mime[".jpg"] = "image/jpeg";
        mime[".jpeg"] = "image/jpeg";
        mime[".gif"] = "image/gif";
        mime[".ico"] = "image/x-icon";
        mime[".svg"] = "image/svg+xml";
        mime[".webp"] = "image/webp";
        mime[".bmp"] = "image/bmp";
        mime[".mp4"] = "video/mp4";
        mime[".mpeg"] = "video/mpeg";
        mime[".mp3"] = "audio/mpeg";
        mime[".wav"] = "audio/wav";
        mime[".ogg"] = "audio/ogg";
        mime[".woff"] = "font/woff";
        mime[".woff2"] = "font/woff2";
        mime[".ttf"] = "font/ttf";
        mime[".otf"] = "font/otf";
        mime[".eot"] = "application/vnd.ms-fontobject";
        mime[".zip"] = "application/zip";
        mime[".tar"] = "application/x-tar";
        mime[".gz"] = "application/gzip";
        mime[".pdf"] = "application/pdf";
        mime[".csv"] = "text/csv";
    }
    return mime;
}

