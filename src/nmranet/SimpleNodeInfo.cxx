/** \copyright
 * Copyright (c) 2014, Balazs Racz
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are  permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \file SimpleNodeInfo.hxx
 *
 * Handler for the Simple Node Ident Info protocol.
 *
 * @author Balazs Racz
 * @date 24 Jul 2013
 */

#include "nmranet/SimpleNodeInfo.hxx"

namespace nmranet
{

extern const SimpleNodeStaticValues __attribute__((weak)) SNIP_STATIC_DATA = {
    4, "OpenMRN", "Undefined model", "Undefined HW version", "0.9"};

const SimpleInfoDescriptor SNIPHandler::SNIP_RESPONSE[] = {
    {SimpleInfoDescriptor::LITERAL_BYTE, 4, 0, nullptr},
    {SimpleInfoDescriptor::C_STRING, 0, 0, SNIP_STATIC_DATA.manufacturer_name},
    {SimpleInfoDescriptor::C_STRING, 0, 0, SNIP_STATIC_DATA.model_name},
    {SimpleInfoDescriptor::C_STRING, 0, 0, SNIP_STATIC_DATA.hardware_version},
    {SimpleInfoDescriptor::C_STRING, 0, 0, SNIP_STATIC_DATA.software_version},
    {SimpleInfoDescriptor::FILE_LITERAL_BYTE, 2, 0, SNIP_DYNAMIC_FILENAME},
    {SimpleInfoDescriptor::FILE_C_STRING, 63, 1, SNIP_DYNAMIC_FILENAME},
    {SimpleInfoDescriptor::FILE_C_STRING, 64, 64, SNIP_DYNAMIC_FILENAME},
    {SimpleInfoDescriptor::END_OF_DATA, 0, 0, 0}};

void init_snip_user_file(int fd, const char *user_name,
                         const char *user_description)
{
    ::lseek(fd, 0, SEEK_SET);
    SimpleNodeDynamicValues data;
    memset(&data, 0, sizeof(data));
    data.version = 2;
    strncpy(data.user_name, user_name, sizeof(data.user_name));
    strncpy(data.user_description, user_description,
            sizeof(data.user_description));
    int ofs = 0;
    auto *p = (const uint8_t *)&data;
    const int len = sizeof(data);
    while (ofs < len)
    {
        int ret = ::write(fd, p, len - ofs);
        if (ret < 0)
        {
            LOG(FATAL, "Init SNIP file: Could not write to fd %d: %s", fd,
                strerror(errno));
        }
        ofs += ret;
    }
}

MockSNIPUserFile::MockSNIPUserFile(const TempDir &dir, const char *user_name,
                                   const char *user_description)
    : userFile_(dir, "snip_user_file")
{
    init_snip_user_file(userFile_.fd(), user_name, user_description);
    HASSERT(userFile_.name().size() < sizeof(snip_user_file_path));
    strncpy(snip_user_file_path, userFile_.name().c_str(),
            sizeof(snip_user_file_path));
}

MockSNIPUserFile::~MockSNIPUserFile() {}

char MockSNIPUserFile::snip_user_file_path[128] = "/dev/zero";

} // namespace nrmanet
