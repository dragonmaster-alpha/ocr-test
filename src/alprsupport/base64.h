
#ifndef OPENALPR_BASE64_H
#define	OPENALPR_BASE64_H

#include <string>
#include "exports.h"

OPENALPRSUPPORT_DLL_EXPORT std::string base64_encode(unsigned char const* , unsigned int len);
OPENALPRSUPPORT_DLL_EXPORT std::string base64_decode(std::string const& s);


OPENALPRSUPPORT_DLL_EXPORT char *base64_c_encode(const unsigned char *data,
                    size_t input_length,
                    size_t *output_length);

OPENALPRSUPPORT_DLL_EXPORT unsigned char *base64_c_decode(const char *data,
                             size_t input_length,
                             size_t *output_length);

#endif	/* OPENALPR_BASE64_H */
