#pragma once

#include "RHI_Definition.h"

namespace PlayGround
{
    // 물리 디바이스
	class PhysicalDevice
	{
    public:
        PhysicalDevice(const uint32_t api_version, const uint32_t driver_version, const uint32_t vendor_id, const RHI_PhysicalDevice_Type type, const char* name, const uint64_t memory, void* data)
        {
            this->vendor_id = vendor_id;
            this->type = type;
            this->name = name;
            this->memory = static_cast<uint32_t>(memory / 1024 / 1024); // mb
            this->data = data;
            this->api_version = decode_driver_version(api_version);
            this->driver_version = decode_driver_version(driver_version);
        }

        /*
            0x10DE - Nvidia
            0x8086 - Intel
            0x1002 - Amd
            0x13B5 - ARM
            0x5143 - Qualcomm
            0x1010 - ImgTec

        */
        inline bool IsNvidia()     const { return vendor_id == 0x10DE || name.find("Nvidia") != std::string::npos; }
        inline bool IsAmd()        const { return vendor_id == 0x1002 || vendor_id == 0x1022 || name.find("Amd") != std::string::npos; }
        inline bool IsIntel()      const { return vendor_id == 0x8086 || vendor_id == 0x163C || vendor_id == 0x8087 || name.find("Intel") != std::string::npos; }
        inline bool IsArm()        const { return vendor_id == 0x13B5 || name.find("Arm,") != std::string::npos; }
        inline bool IsQualcomm()   const { return vendor_id == 0x5143 || name.find("Qualcomm") != std::string::npos; }

        inline const std::string& GetName()            const { return name; }
        inline const std::string& GetDriverVersion()   const { return driver_version; }
        inline const std::string& GetApiVersion()      const { return api_version; }
        inline uint32_t GetMemory()                    const { return memory; }
        inline void* GetData()                         const { return data; }

    private:
        std::string decode_driver_version(const uint32_t version)
        {
            char buffer[256];

            if (IsNvidia())
            {
                sprintf_s
                (
                    buffer,
                    "%d.%d.%d.%d",
                    (version >> 22) & 0x3ff,
                    (version >> 14) & 0x0ff,
                    (version >> 6) & 0x0ff,
                    (version) & 0x003f
                );

            }
            else if (IsIntel())
            {
                sprintf_s
                (
                    buffer,
                    "%d.%d",
                    (version >> 14),
                    (version) & 0x3fff
                );
            }
            else
            {
                sprintf_s
                (
                    buffer,
                    "%d.%d.%d",
                    (version >> 22),
                    (version >> 12) & 0x3ff,
                    version & 0xfff
                );
            }

            return buffer;
        }

        std::string api_version = "Unknown";
        std::string driver_version = "Unknown";
        uint32_t vendor_id = 0;
        RHI_PhysicalDevice_Type type = RHI_PhysicalDevice_Type::Unknown;
        std::string name = "Unknown";
        uint32_t memory = 0;
        void* data = nullptr;
	};
}