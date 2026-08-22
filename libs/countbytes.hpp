#pragma once
#include <string>
struct SizeBytes;
class CountBytes {
private:
    size_t countbytes;
public:
    CountBytes(size_t cb): countbytes(cb) {}
    size_t to_bits() const {
        return countbytes*8;
    }
    size_t to_bytes() const {
        return countbytes;
    }
    size_t to_kilobytes() const {
        return countbytes/1024;
    }
    float to_kilobytesf() const {
        return countbytes/1024.0;
    }
    size_t to_megabytes() const {
        return to_kilobytesf()/1024;
    }
    float to_megabytesf() const {
        return to_kilobytesf()/1024;
    }
    size_t to_gigabytes() const {
        return to_megabytesf()/1024;
    }
    float to_gigabytesf() const {
        return to_megabytesf()/1024;
    }
    SizeBytes toSizeBytes() const;
    std::wstring toString1valW() const;
    std::string toString2val() const;
    std::string toString1val() const;
    std::wstring toString2valW() const;
};
struct SizeBytes {
    int16_t countbytes;
    int16_t countkilobytes;
    int16_t countmegabytes;
    int32_t countgigabytes;
    size_t to_bytes() {
        return countbytes+countkilobytes*1024+countmegabytes*1024*1024+countgigabytes*1024*1024*1024;
    }
    CountBytes toCountBytes() {
        return CountBytes(to_bytes());
    }
    std::string toString1val() {
        if (countgigabytes!=0) {
            size_t val = ((float)countgigabytes+float(countmegabytes)/1024)*10;
            return std::to_string(val/10)+"."+std::to_string(val%10)+"G";
        } else if (countmegabytes!=0) {
            size_t val = ((float)countmegabytes+float(countkilobytes)/1024)*10;
            return std::to_string(val/10)+"."+std::to_string(val%10)+"M";
        } else if (countkilobytes!=0) {
            size_t val = ((float)countkilobytes+float(countbytes)/1024)*10;
            return std::to_string(val/10)+"."+std::to_string(val%10)+"K";
        } else {
            return std::to_string(countbytes)+"b";
        }
    }
    std::wstring toString1valW() {
        if (countgigabytes!=0) {
            size_t val = ((float)countgigabytes+float(countmegabytes)/1024)*10;
            return std::to_wstring(val/10)+L"."+std::to_wstring(val%10)+L" ГГ";
        } else if (countmegabytes!=0) {
            size_t val = ((float)countmegabytes+float(countkilobytes)/1024)*10;
            return std::to_wstring(val/10)+L"."+std::to_wstring(val%10)+L" МБ";
        } else if (countkilobytes!=0) {
            size_t val = ((float)countkilobytes+float(countbytes)/1024)*10;
            return std::to_wstring(val/10)+L"."+std::to_wstring(val%10)+L" КБ";
        } else {
            return std::to_wstring(countbytes)+L" байт";
        }
    }
    std::string toString2val() {
        if (countgigabytes!=0) {
            std::string str = std::to_string(countgigabytes)+"G";
            if (countmegabytes!=0) {
                str += " "+std::to_string(countmegabytes)+"M";
            }
            return str;
        } else if (countmegabytes!=0) {
            std::string str = std::to_string(countmegabytes)+"M";
            if (countkilobytes!=0) {
                str += " "+std::to_string(countkilobytes)+"K";
            }
            return str;
        } else if (countkilobytes!=0) {
            std::string str = std::to_string(countkilobytes)+"K";
            if (countbytes!=0) {
                str += " "+std::to_string(countbytes)+"b";
            }
            return str;
        } else {
            return std::to_string(countbytes)+"b";
        }
    }
    std::wstring toString2valW() {
        if (countgigabytes!=0) {
            std::wstring str = std::to_wstring(countgigabytes)+L" ГГ";
            if (countmegabytes!=0) {
                str += L" "+std::to_wstring(countmegabytes)+L" МБ";
            }
            return str;
        } else if (countmegabytes!=0) {
            std::wstring str = std::to_wstring(countmegabytes)+L" МБ";
            if (countkilobytes!=0) {
                str += L" "+std::to_wstring(countkilobytes)+L" КБ";
            }
            return str;
        } else if (countkilobytes!=0) {
            std::wstring str = std::to_wstring(countkilobytes)+L" KБ";
            if (countbytes!=0) {
                str += L" "+std::to_wstring(countbytes)+L" байт";
            }
            return str;
        } else {
            return std::to_wstring(countbytes)+L" байт";
        }
    }
};
SizeBytes CountBytes::toSizeBytes() const {
    int16_t cbytes = countbytes%1024;
    int16_t kilobytes = ((countbytes-cbytes)/1024)%1024;
    int16_t megabytes = ((countbytes-cbytes-kilobytes*1024)/1024/1024)%1024;
    int32_t gigabytes = (countbytes-cbytes-kilobytes*1024-megabytes*1024*1024)/1024/1024/1024;
    return SizeBytes(cbytes, kilobytes, megabytes, gigabytes);
}
std::string CountBytes::toString1val() const {
    return toSizeBytes().toString1val();
}
std::wstring CountBytes::toString1valW() const {
    return toSizeBytes().toString1valW();
}
std::string CountBytes::toString2val() const {
    return toSizeBytes().toString2val();
}
std::wstring CountBytes::toString2valW() const {
    return toSizeBytes().toString2valW();
}
