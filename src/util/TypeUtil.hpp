#pragma once

template <typename T, typename P> inline bool instanceof(const P p) {
    return dynamic_cast<const T*>(p) != 0;
}
