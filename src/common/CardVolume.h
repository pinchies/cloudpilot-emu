#ifndef _CARD_VOLUME_H_
#define _CARD_VOLUME_H_

#include <cstdint>
#include <string>

#include "CardImage.h"

class CardVolume {
   public:
    enum class Type : uint8_t { partition, bigFloppy, invalid };

   public:
    explicit CardVolume(CardImage& image);

    Type GetType() const;
    const std::string& InvalidReason() const;

    uint32_t GetSize() const;
    uint32_t GetGeometryHeads() const;
    uint32_t GetGeometrySectors() const;
    uint32_t GetSectorsPerCluster() const;

   private:
    struct AddressCHS {
        uint16_t cylinder;
        uint8_t head;
        uint8_t sector;
    };

   private:
    void Identify();

    uint8_t Read8(uint32_t addr) const;
    uint16_t Read16(uint32_t addr) const;
    uint32_t Read32(uint32_t addr) const;

    bool ReadPartition(uint8_t index);
    void ReadFatParameters();
    AddressCHS ReadAddressCHS(uint32_t index);

    uint32_t CHSToLBA(const AddressCHS& addressCHS) const;

   private:
    uint8_t* imageData;
    uint32_t imageSize{0};

    Type type{Type::invalid};
    std::string invalidReason{""};

    uint32_t partitionFirstByte{0};
    uint32_t partitionSize{0};

    uint32_t geometryHeads{0};
    uint32_t geometrySectors{0};
    uint32_t sectorsPerCluster{0};
};

#endif  // _CARD_VOLUME_H_
