#include "xrdndndpdk-utils.h"

#include <stdio.h>

/**
 * @brief TLV Name component representation
 *
 */
typedef struct NameComponent {
    uint16_t type;
    uint16_t length;
    uint8_t *value;
} NameComponent;

/**
 * @brief All Name components in Packet LName
 *
 */
typedef struct NameComponents {
    NameComponent components[NAME_MAX_LENGTH];
    uint16_t size;
} NameComponents;


static uint64_t getLength(const uint8_t *buff, uint16_t *offset) {
    uint64_t TLV_LENGTH = 0;

    if (buff[*offset] <= 0xFC) { // Length value is encoded in this octet
        TLV_LENGTH |= buff[(*offset)++];
    } else if (buff[*offset] ==
               0xFD) { // Length value is encoded in the following 2 octets
        *offset += 1;
        TLV_LENGTH |= buff[(*offset)++];
        TLV_LENGTH <<= 8;
        TLV_LENGTH |= buff[(*offset)++];
        assert(TLV_LENGTH > 0xFC);
    } else if (buff[*offset] ==
               0xFE) { // Length value is encoded in the following 4 octets
        *offset += 1;
        TLV_LENGTH |= buff[(*offset)++];
        TLV_LENGTH <<= 8;
        TLV_LENGTH |= buff[(*offset)++];
        TLV_LENGTH <<= 8;
        TLV_LENGTH |= buff[(*offset)++];
        TLV_LENGTH <<= 8;
        TLV_LENGTH |= buff[(*offset)++];
        assert(TLV_LENGTH > 0xFFFF);
    } else if (buff[*offset] ==
               0xFF) { // Length value is encoded in the following 8 octets
        *offset += 1;
        TLV_LENGTH |= buff[(*offset)++];
        TLV_LENGTH <<= 8;
        TLV_LENGTH |= buff[(*offset)++];
        TLV_LENGTH <<= 8;
        TLV_LENGTH |= buff[(*offset)++];
        TLV_LENGTH <<= 8;
        TLV_LENGTH |= buff[(*offset)++];
        TLV_LENGTH <<= 8;
        TLV_LENGTH |= buff[(*offset)++];
        TLV_LENGTH <<= 8;
        TLV_LENGTH |= buff[(*offset)++];
        TLV_LENGTH <<= 8;
        TLV_LENGTH |= buff[(*offset)++];
        TLV_LENGTH <<= 8;
        TLV_LENGTH |= buff[(*offset)++];
        assert(TLV_LENGTH > 0xFFFFFFFF);
    } else {
        exit(EXIT_FAILURE);
    }

    return TLV_LENGTH;
}

static NameComponents *getNameComponents(const LName name,
                                         uint16_t startOffset) {
    NameComponents *c = rte_malloc(NULL, sizeof(NameComponents), 0);
    c->size = 0;

    for (uint16_t i = startOffset; i < name.length;) {
        c->components[c->size].type = name.value[i++];
        assert(c->components[c->size].type == TT_GenericNameComponent);

        c->components[c->size].length = getLength(name.value, &i);

        c->components[c->size].value =
            rte_malloc(NULL, c->components[c->size].length, 0);
        rte_memcpy(c->components[c->size].value, &name.value[i],
                   c->components[c->size].length);
        i += c->components[c->size].length;
        ++c->size;
    }

    return c;
}

char *lnameGetFilePath(const LName lname, uint16_t lnameOff,
                       bool hasSegmentNo) {
    NameComponents *nameComponents = getNameComponents(lname, lnameOff);

    char *filePath = rte_malloc(NULL, NAME_MAX_LENGTH * sizeof(char), 0);
    int nComponents =
        likely(hasSegmentNo) ? nameComponents->size - 1 : nameComponents->size;

    for (int i = 0; i < nComponents; ++i) {
        strcat(filePath, "/");
        strcat(filePath, nameComponents->components[i].value);
    }

    for (int i = 0; i < nameComponents->size; ++i) {
        rte_free(nameComponents->components[i].value);
    }

    rte_free(nameComponents);

    return filePath;
}

SystemCallId lnameGetSystemCallId(const LName name) {
    if (likely(XRDNDNDPDK_SYCALL_PREFIX_READ_ENCODE_SIZE <= name.length &&
               memcmp(XRDNDNDPDK_SYCALL_PREFIX_READ_ENCODE, name.value,
                      XRDNDNDPDK_SYCALL_PREFIX_READ_ENCODE_SIZE) == 0)) {
        return SYSCALL_READ_ID;
    } else if (XRDNDNDPDK_SYCALL_PREFIX_OPEN_ENCODE_SIZE <= name.length &&
               memcmp(XRDNDNDPDK_SYCALL_PREFIX_OPEN_ENCODE, name.value,
                      XRDNDNDPDK_SYCALL_PREFIX_OPEN_ENCODE_SIZE) == 0) {
        return SYSCALL_OPEN_ID;
    } else if (XRDNDNDPDK_SYCALL_PREFIX_FSTAT_ENCODE_SIZE <= name.length &&
               memcmp(XRDNDNDPDK_SYCALL_PREFIX_FSTAT_ENCODE, name.value,
                      XRDNDNDPDK_SYCALL_PREFIX_FSTAT_ENCODE_SIZE) == 0) {
        return SYSCALL_FSTAT_ID;
    }

    return SYSCALL_NOT_FOUND;
}

PContent packetGetContent(uint8_t *packet, uint16_t len) {
    assert(packet[0] == TT_Data);

    uint8_t TLV_TYPE = 0;
    uint64_t TLV_LENGTH = 0;
    uint16_t offset = 0;

    for (; offset < len;) {
        TLV_TYPE = packet[offset++];
        assert(TLV_TYPE == TT_Data || TLV_TYPE == TT_Name ||
               TLV_TYPE == TT_MetaInfo || TLV_TYPE == TT_Content);

        TLV_LENGTH = getLength(packet, &offset);

        if (TLV_TYPE == TT_Content)
            break; // Found Content in packet
        if (TLV_TYPE != TT_Data) {
            offset += TLV_LENGTH; // Skip this type's length until Content
        }
    }

    PContent content = {
        .type = TLV_TYPE, .length = TLV_LENGTH, .offset = offset, .buff = NULL};
    return content;
}