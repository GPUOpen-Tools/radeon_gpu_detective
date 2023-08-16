//=============================================================================
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief  serializer for memory resource information.
//=============================================================================

#include <unordered_map>
#include <string>
#include <sstream>
#include <memory>
#include <cassert>
#include <iomanip>

// RGD
#include "rgd_resource_info_serializer.h"
#include "rgd_utils.h"
#include "rgd_parsing_utils.h"
#include "rgd_data_types.h"

// RMV
#include "rmv/source/backend/rmt_memory_event_history.h"
#include "rmv/source/backend/rmt_data_set.h"
#include "rmv/source/parser/rmt_types.h"
#include "rmv/source/parser/rmt_print.h"
#include "rmv/source/backend/rmt_memory_event_history_impl.h"
#include "rmv/source/backend/rmt_trace_loader.h"

// JSON.
#include "json/single_include/nlohmann/json.hpp"

// *** INTERNALLY-LINKED AUXILIARY CONSTANTS - BEGIN ***

static const char* kResourceTypeBufferStr = "Buffer";
static const char* kResourceTypeImageStr = "Image";
static const char* kResourceTypePipelineStr = "Pipeline";
static const char* kResourceTypeCommandBufferStr = "Command Buffer";
static const char* kResourceTypeHeapStr = "Heap";
static const char* kResourceTypeDesctiptorStr = "Descriptor";
static const char* kResourceTypeGpuEventStr = "GPU Event";
static const char* kResourceTypeInternalStr = "Internal";
static const char* kResourceTypeUnknownStr = "Unknown";
static const char* kResourceEventTypeCreateStr = "Create";
static const char* kResourceEventTypeBindStr = "Bind";
static const char* kResourceEventTypeMakeResidentStr = "Make Resident";
static const char* kResourceEventTypeEvictStr = "Evict";
static const char* kResourceEventTypeDestroy = "Destroy";
static const char* kPrintRawTimestampMsg = "Invalid time frequency information received from the input crash dump file. Raw timestamps will be printed in text output.";
static const char* kNullStr = "NULL";

static const int kMinEventsToExpand = 2;
static const int kTimestampWidth = 21;

// *** INTERNALLY-LINKED AUXILIARY CONSTANTS - ENDS ***

// *** INTERNALLY-LINKED AUXILIARY FUNCTIONS - BEGIN ***

template<typename T> static void PrintFormattedResourceTimeline(T t, const int width, std::stringstream& txt)
{
    txt << std::left << std::setw(width) << std::setfill(' ') << t;
}

// Get virtual address string.
static std::string ToAddressString(uint64_t virtual_address)
{
    std::stringstream txt;
    txt << "Address: 0x" << std::hex << virtual_address << std::dec << " ";
    return txt.str();
}

// Get the preferred heap string.
static std::string ToPreferredHeapString(RmtHeapType heap_type)
{
    std::string heap_type_str;
    switch (heap_type)
    {
    case kRmtHeapTypeLocal:
        heap_type_str = kStrHeapTypeLocal;
        break;

    case kRmtHeapTypeInvisible:
        heap_type_str = kStrHeapTypeInvisible;
        break;

    case kRmtHeapTypeSystem:
        heap_type_str = kStrHeapTypeHost;
        break;

    case kRmtHeapTypeNone:
        heap_type_str = "Unspecified";
        break;

    default:
        assert(false);
        heap_type_str = "Unknown";
    }

    return heap_type_str;
}

static const uint32_t kMaxFlagsStringLength = 512;

// Get commit type string.
static const std::string GetCommitTypeStringFromCommitType(RmtCommitType commit_type)
{
    return  RmtGetCommitTypeNameFromCommitType(commit_type);
}

// Get image type string.
static const std::string GetImageTypeStringFromImageType(RmtImageType image_type)
{
    return  RmtGetImageTypeNameFromImageType(image_type);
}

// Get image swizzle pattern string.
static const std::string GetSwizzlePatternString(const RmtImageFormat* image_format)
{
    char flags_text[kMaxFlagsStringLength] = "";
    return  RmtGetSwizzlePatternFromImageFormat(image_format, flags_text, kMaxFlagsStringLength);;
}

// Get image format string.
static const std::string GetImageFormatString(const RmtFormat image_format)
{
    return  RmtGetFormatNameFromFormat(image_format);;
}

// Get image tiling type string.
static const std::string GetImageTypeStringFromImageType(RmtTilingType tiling_type)
{
    return  RmtGetTilingNameFromTilingType(tiling_type);
}

// Get flags string.
static const std::string GetFlagsString(RmtResourceType resource_type, uint32_t flags, std::string flag_type = "create")
{
    char flags_text[kMaxFlagsStringLength] = "";

    switch (resource_type)
    {
    case kRmtResourceTypeBuffer:
        if (flag_type == "create")
        {
            RmtGetBufferCreationNameFromBufferCreationFlags(flags, flags_text, kMaxFlagsStringLength);
        }
        else if (flag_type == "usage")
        {
            RmtGetBufferUsageNameFromBufferUsageFlags(flags, flags_text, kMaxFlagsStringLength); (flags, flags_text, kMaxFlagsStringLength);
        }
        else
        {
            // should not reach here.
            assert(false);
        }
        break;

    case kRmtResourceTypeImage:
        if (flag_type == "create")
        {
            RmtGetImageCreationNameFromImageCreationFlags(flags, flags_text, kMaxFlagsStringLength);
        }
        else if (flag_type == "usage")
        {
            RmtGetImageUsageNameFromImageUsageFlags(flags, flags_text, kMaxFlagsStringLength);
        }
        else
        {
            // should not reach here.
            assert(false);
        }
        break;

    case kRmtResourceTypePipeline:
        RmtGetPipelineCreationNameFromPipelineCreationFlags(flags, flags_text, kMaxFlagsStringLength);
        break;

    case kRmtResourceTypeCommandAllocator:
        RmtGetCmdAllocatorNameFromCmdAllocatorFlags(flags, flags_text, kMaxFlagsStringLength);
        break;

    case kRmtResourceTypeGpuEvent:
        RmtGetGpuEventNameFromGpuEventFlags(flags, flags_text, kMaxFlagsStringLength);
        break;

    default:
        // Should not reach here.
        assert(false);
        break;
    }
    return flags_text;

}

// Get a resource type as a string.
static const char* GetResourceTypeText(const RmtResourceType resource_type)
{
    const char* resource_type_string;
    switch (resource_type)
    {
    case kRmtResourceTypeBuffer:
        resource_type_string = kResourceTypeBufferStr;
        break;

    case kRmtResourceTypeImage:
        resource_type_string = kResourceTypeImageStr;
        break;

    case kRmtResourceTypePipeline:
        resource_type_string = kResourceTypePipelineStr;
        break;

    case kRmtResourceTypeCommandAllocator:
        resource_type_string = kResourceTypeCommandBufferStr;
        break;

    case kRmtResourceTypeHeap:
        resource_type_string = kResourceTypeHeapStr;
        break;

    case kRmtResourceTypeDescriptorHeap:
    case kRmtResourceTypeDescriptorPool:
        resource_type_string = kResourceTypeDesctiptorStr;
        break;

    case kRmtResourceTypeGpuEvent:
        resource_type_string = kResourceTypeGpuEventStr;
        break;

    case kRmtResourceTypeBorderColorPalette:
    case kRmtResourceTypeTimestamp:
    case kRmtResourceTypeMiscInternal:
    case kRmtResourceTypePerfExperiment:
    case kRmtResourceTypeMotionEstimator:
    case kRmtResourceTypeVideoDecoder:
    case kRmtResourceTypeVideoEncoder:
    case kRmtResourceTypeQueryHeap:
    case kRmtResourceTypeIndirectCmdGenerator:
        resource_type_string = kResourceTypeInternalStr;
        break;

    default:
        assert(false);
        resource_type_string = kResourceTypeUnknownStr;
    }

    return resource_type_string;
}

// *** INTERNALLY-LINKED AUXILIARY FUNCTIONS - END ***

enum class RgdResourceEventType {kCreate, kBind, kMakeResident, kEvict, kDestroy};

struct RgdResourceTimeline
{
    RgdResourceTimeline(RgdResourceEventType memory_event_type, RmtResourceIdentifier resource_id, uint64_t timestamp) : event_type(memory_event_type),
        identifier(resource_id), event_timestamp(timestamp){}
    RgdResourceEventType event_type = RgdResourceEventType::kCreate;
    RmtResourceIdentifier identifier = 0;
    uint64_t event_timestamp = 0;

    // The virtual address associated with the bind event.
    uint64_t bound_virtual_address = 0;

    // The parent allocation base address for this bind event.
    uint64_t bound_base_address = 0;

    // The size associated with the bind event.
    uint64_t bound_size_in_bytes = 0;

    // The heap preferences for the virtual allocation received with the bind event.
    RmtHeapType heap_preferences[RMT_NUM_HEAP_PREFERENCES];
};

struct RgdResource
{
    // The resource name.
    std::string resource_name;

    // A GUID for this resource.
    RmtResourceIdentifier rmv_identifier = 0;

    // A resource_list_[index] provides a pointer to the the RgdResource.
    // If there exists an implicit resource created along with this resource,
    // associated_resource_idx will store the index of pairing resource.
    intmax_t associated_resource_idx = -1;

    // Resource timeline event indices for the resource.
    std::vector<size_t>   timeline_indices;

    // The time the resource was destroyed.
    uint64_t              destroyed_time = 0;

    // The virtual address of the resource.
    // In case of multiple bind events (for Heap), this holds the address from the most recent bind event.
    uint64_t              address = 0;

    // The parent allocation base address.
    // In case of multiple bind events (for Heap), this holds the address from the most recent bind event.
    uint64_t              allocation_base_address = 0;

    // The total size of the resource.
    // In case of multiple bind events (for Heap), this holds the size from the most recent bind event.
    uint64_t              size_in_bytes = 0;

    // The offset from the base address.
    uint64_t              allocation_offset = 0;

    // The commit type of the resource.
    RmtCommitType         commit_type;

    // The type of the resource.
    RmtResourceType       resource_type;

    union
    {
        // Valid when resourceType is kRmtResourceTypeImage.
        RmtResourceDescriptionImage    image{ 0 };

        // Valid when is kRmtResourceTypeBuffer.
        RmtResourceDescriptionBuffer   buffer;

        // Valid when resourceType is kRmtResourceTypePipeline.
        RmtResourceDescriptionPipeline pipeline;

        // Valid when resourceType is kRmtResourceTypeGpuEvent.
        RmtResourceDescriptionGpuEvent gpu_event;

        // Valid when resourceType is RMT_RESOURCE_TYPE_COMMAND_ALLOCATOR.
        RmtResourceDescriptionCommandAllocator
            command_allocator;
    };
};


struct RgdVaInfo
{
    // Map to store resource information.
    std::unordered_map<RmtResourceIdentifier, std::shared_ptr<RgdResource>> resource_map_;

    // Vector to store pointers to RgdResource in the order of the creation timestamp.
    std::vector<std::shared_ptr<RgdResource>> resource_list_;

    // Virtual Address made resident and evict event info serialized string.
    std::string va_residency_info_;

    // Virtual Address memory timeline event info json.
    nlohmann::json va_residency_json_;

    // Resource create, bind and destroy timeline.
    std::vector<RgdResourceTimeline> rgd_resource_timeline_;

    // Sort rgd resource timeline.
    void SortResourceTimeline();
};

class RgdResourceInfoSerializer::pImplResourceInfoSerializer
{
public:

    /// @brief Initialize data set handle.
    ///
    /// @param [in] trace_file_name
    ///
    /// @return true when initializatoin is successful; false otherwise.
    bool InitializeDataSet(const std::string& trace_file_name);

    /// @brief Serialize resource info from resource_map_ to string.
    ///
    /// @param [in]  virtual_address    The Gpu address.
    /// @param [out] resource_info_text Serialized resource information string.
    void ResourceHistoryToString(const uint64_t virtual_address,
        std::string& resource_info_text);

    /// @brief Resource history info from resource_map to json.
    ///
    /// @param [in]  virtual_address    The Gpu address.
    /// @param [out] resource_info_json The resource info json.
    ///
    /// @return true when resource history JSON is built successfully; false otherwise.
    bool ResourceHistoryToJson(const uint64_t virtual_address,
        nlohmann::json& resource_info_json);

    /// @brief Build resource history from a memory event history.
    ///
    /// @param [in] virtual_address     The Gpu address.
    /// @param [in] history_handle      Event history handle.
    /// @param [in] event_index         Event index from list of events.
    ///
    /// @return true when resource info is build successfully; false otherwise.
    bool BuildResourceMapFromEvent(const uint64_t virtual_address,
        const RmtMemoryEventHistoryHandle     history_handle,
        const RmtMemoryEventHistoryEventIndex event_index);

    /// @brief Build resource history for the virtual address from memory event history data.
    ///
    /// This functions initializes the dataset handle for the input trace file, then builds
    /// the resource history data structure - resource_map_ and virtual address memory residency
    /// information in string and/or json as per user config.
    ///
    /// @param [in] user_config     The input user configuration.
    /// @param [in] virtual_address The GPU address.
    ///
    /// @return
    bool BuildResourceHistoryforVa(const Config& user_config,
        const uint64_t virtual_address);

    /// @brief Generate resource timeline.
    ///
    /// @param [in]  virtual_address   The Gpu address.
    /// @param [out] resource_timeline The resource timeline.
    void GenerateResourceTimeline(const uint64_t virtual_address, std::string& resource_timeline);

    /// @brief Get the offset of the resource virtual address from allocation base address.
    ///
    /// @param [in] allocation_base_address  The parent allocation base address.
    /// @param [in] resource_virtual_address The virtual address of the resource.
    ///
    /// @return offset from the base address.
    uint64_t GetAllocationOffset(uint64_t allocation_base_address, uint64_t resource_virtual_address);

    /// @brief Report output format requirement.
    ///
    /// @return true if text output is expected; false otherwise.
    bool IsTextRequired();

    /// @brief Report output format requirement.
    ///
    /// @return true if json output is expected; false otherwise.
    bool IsJsonRequired();

    /// @brief Get real time string.
    ///
    /// @param [in] timestamp The raw timestamp.
    ///
    /// @return real time string.
    std::string GetRealTime(uint64_t timestamp);

    /// @brief Get the timestamp string.
    ///
    /// @param [in] timestamp The raw timestamp.
    ///
    /// @return raw or real time string.
    std::string GetTimestampString(const uint64_t timestamp);

    /// @brief Get the crashing applications process id.
    ///
    /// @return the crashing applications process id.
    uint64_t GetTargetProcessId() const;

    // Map to store virtual address specific information.
    std::unordered_map<RmtGpuAddress, std::unique_ptr<RgdVaInfo>> va_info_map_;

private:
    /// @brief Set output format.
    ///
    /// @param [in] user_config The user configuration.
    void SetOutputFormat(const Config& user_config);

    /// @brief Set cpu frequency information of the crash analysis session.
    ///
    /// @param [in] cpu_frequency The cpu frequency of the crash analysis session from the input crash dump file.
    void SetCpuFrequency(const uint64_t cpu_frequency);

    /// @brief Set the crashing applications process id.
    ///
    /// @param [in] target_process_id The process id.
    void SetTargetProcessId(const uint64_t target_process_id);

    /// @brief Find and update the associated Rmt resource and its implicit resource pair.
    ///
    /// When RmtResource type 'Buffer' or 'Image' is created, and implicit 'Heap' resource is created which has the same size and the base address.
    /// This function will iterate though the list of all resources and find the pair of RmtResource and the related implicit resource.
    /// If implicit resource found, it will update the the resource info (associated_resource_idx) with the index of the associated implicit resource and the implicit resource info
    /// will be updated with the index of its associated RmtResource. The associated_resource_idx is used to get the pointer of that resources from resource_list_ during serialization.
    ///
    /// @param [in] virtual_address   The Gpu address.
    void FindAndUpdateRmtResourceAndImplicitHeapPair(const uint64_t virtual_address);

    /// @brief Get the resource creation time.
    ///
    /// @param [in] va_info      The RgdVaInfo handle.
    /// @param [in] rgd_resource The RgdResource handle.
    ///
    /// @return resource creation timestamp.
    uint64_t GetResourceCreateTime(const RgdVaInfo& va_info, const RgdResource& rgd_resource) const;

    bool is_text_required_ = false;
    bool is_json_required_ = false;
    bool is_raw_time_ = false;

    // Frequency of timestamps in Rmt data chunk.
    uint64_t cpu_frequency_ = 0;

    // The crashing application process ID that was traced.
    uint64_t target_process_id_ = 0;
};

void RgdVaInfo::SortResourceTimeline()
{
    assert(!rgd_resource_timeline_.empty());

    std::sort(rgd_resource_timeline_.begin(),
        rgd_resource_timeline_.end(),
        [&](const RgdResourceTimeline& a_resource_event, const RgdResourceTimeline& b_resource_event)
        {
            bool ret = false;
            if (a_resource_event.event_timestamp != b_resource_event.event_timestamp)
            {
                ret = (a_resource_event.event_timestamp < b_resource_event.event_timestamp);
            }
            else
            {
                assert(resource_map_[a_resource_event.identifier] != nullptr && resource_map_[b_resource_event.identifier] != nullptr);
                RgdResource& a_rgd_resource = *resource_map_[a_resource_event.identifier];
                RgdResource& b_rgd_resource = *resource_map_[b_resource_event.identifier];

                if (a_resource_event.event_type != b_resource_event.event_type)
                {
                    // Order the events in logical order of the events -> "Create" -> "Bind" -> "Make Resident" -> "Evict" -> "Destroy" when same timestamp.
                    ret = ((static_cast<int>(a_resource_event.event_type)) < (static_cast<int>(b_resource_event.event_type)));
                }
                else
                {
                    if (a_rgd_resource.resource_type == kRmtResourceTypeHeap
                        && (b_rgd_resource.resource_type == kRmtResourceTypeBuffer
                            || b_rgd_resource.resource_type == kRmtResourceTypeImage))
                    {
                        ret = (a_resource_event.event_type == RgdResourceEventType::kCreate || a_resource_event.event_type == RgdResourceEventType::kBind);
                    }
                }
            }
            return ret;
        });

    assert(!resource_list_.empty());

    // Clear timeline indices list if already built.
    for (size_t index = 0; index < resource_list_.size(); index++)
    {
        assert(resource_list_[index] != nullptr);
        RgdResource& rgd_resource = *resource_list_[index];

        rgd_resource.timeline_indices.clear();
    }

    assert(!rgd_resource_timeline_.empty());

    // Update the timeline indices for each resource as sorting of RgdResourceTimeline may cause index change of an event.
    for (size_t index = 0; index < rgd_resource_timeline_.size(); index++)
    {
        assert(&rgd_resource_timeline_[index] != nullptr);
        const RgdResourceTimeline& timeline_event = rgd_resource_timeline_[index];
        assert(resource_map_[timeline_event.identifier] != nullptr);
        RgdResource& current_rgd_resource = *resource_map_[timeline_event.identifier];

        current_rgd_resource.timeline_indices.push_back(index);
    }
}

bool RgdResourceInfoSerializer::pImplResourceInfoSerializer::InitializeDataSet(const std::string& trace_file_name)
{
    bool result = false;

    if (RmtTraceLoaderTraceLoad(trace_file_name.c_str()) == kRmtOk)
    {
        result = true;

        // Set cpu frequency of the crash analysis session (used for converting raw timestamp to real time).
        SetCpuFrequency(RmtTraceLoaderGetDataSet()->cpu_frequency);

        // Set the crashing applications process id.
        SetTargetProcessId(RmtTraceLoaderGetDataSet()->target_process_id);
    }

    return result;
}

RgdResourceInfoSerializer::RgdResourceInfoSerializer()
{
    resource_info_serializer_impl_ = std::make_unique<pImplResourceInfoSerializer>();
}

RgdResourceInfoSerializer::~RgdResourceInfoSerializer()
{
    // Free the existing dataset handle.
    if (RmtTraceLoaderDataSetValid())
    {
        RmtTraceLoaderClearTrace();
    }
}

bool RgdResourceInfoSerializer::pImplResourceInfoSerializer::IsTextRequired()
{
    return is_text_required_;
}

bool RgdResourceInfoSerializer::pImplResourceInfoSerializer::IsJsonRequired()
{
    return is_json_required_;
}

bool RgdResourceInfoSerializer::pImplResourceInfoSerializer::BuildResourceHistoryforVa(const Config& user_config, const uint64_t virtual_address)
{
    SetOutputFormat(user_config);

    bool result = RmtTraceLoaderDataSetValid();
    if (result)
    {
        va_info_map_[virtual_address] = std::make_unique<RgdVaInfo>();
        RmtMemoryEventHistoryHandle history_handle = nullptr;
        RmtErrorCode is_rmt_result = kRmtOk;
        if (virtual_address != 0)
        {
            is_rmt_result = RmtMemoryEventHistoryGenerateFullAllocationHistory(RmtTraceLoaderGetDataSet(), virtual_address, false, false, &history_handle);
        }
        else
        {
            is_rmt_result = RmtMemoryEventHistoryGenerateHistoryForAllResources(RmtTraceLoaderGetDataSet(), &history_handle);
        }

        if (is_rmt_result == kRmtOk && history_handle != nullptr)
        {
            EventHistoryImpl* history = reinterpret_cast<EventHistoryImpl*>(history_handle);

            assert(history != nullptr);
            RmtResourceHistoryEventType event_type = static_cast<RmtResourceHistoryEventType>(0);
            if (history != nullptr)
            {
                size_t event_count = 0;
                RmtMemoryEventHistoryGetEventCount(history_handle, &event_count);

                // Loop through all history events
                for (RmtMemoryEventHistoryEventIndex index = 0; index < event_count; index++)
                {
                    is_rmt_result = RmtMemoryEventHistoryGetEventType(history_handle, index, &event_type);
                    if (is_rmt_result == kRmtOk)
                    {
                        result = BuildResourceMapFromEvent(virtual_address, history_handle, index);
                    }
                    else
                    {
                        result = false;
                        break;
                    }
                }
            }
            else
            {
                result = false;
            }
            RmtMemoryEventHistoryFreeHistory(&history_handle);
        }
        else
        {
            // Shouldn't get here.
            assert(false);
            result = false;
        }

        if (result && !user_config.is_expand_implicit_resources)
        {
            // Find and update the associated implicit heap and RmtResource pair if exists.
            FindAndUpdateRmtResourceAndImplicitHeapPair(virtual_address);
        }
    }
    return result;
}

uint64_t RgdResourceInfoSerializer::pImplResourceInfoSerializer::GetAllocationOffset(uint64_t allocation_base_address, uint64_t resource_virtual_address)
{
    assert(resource_virtual_address >= allocation_base_address);
    return resource_virtual_address - allocation_base_address;
}

bool RgdResourceInfoSerializer::InitializeWithTraceFile(const std::string& trace_file_path)
{
    bool result = false;

    // Free the existing dataset handle.
    if (RmtTraceLoaderDataSetValid())
    {
        RmtTraceLoaderClearTrace();
    }
    result = resource_info_serializer_impl_->InitializeDataSet(trace_file_path);

    return result;
}

uint64_t RgdResourceInfoSerializer::GetCrashingProcessId() const
{
    return resource_info_serializer_impl_->GetTargetProcessId();
}

bool RgdResourceInfoSerializer::GetVirtualAddressHistoryInfo(const Config& user_config, const uint64_t virtual_address, nlohmann::json& out_json)
{
    bool result = false;
    if (resource_info_serializer_impl_->va_info_map_.find(virtual_address) == resource_info_serializer_impl_->va_info_map_.end())
    {
        //Resource history for the VA is not built. Build resource history.
        result = resource_info_serializer_impl_->BuildResourceHistoryforVa(user_config, virtual_address);
    }
    else
    {
        //Resource history for the VA is built already.
        result = true;
    }

    if (result)
    {
        if (resource_info_serializer_impl_->IsJsonRequired())
        {
            // VA timeline not available for 'all-resources' option.
            if (user_config.is_va_timeline && virtual_address!= kVaReserved)
            {
                out_json[kJsonElemPageFaultSummary]["va_timeline"] = resource_info_serializer_impl_->va_info_map_[virtual_address]->va_residency_json_;
            }
            result = resource_info_serializer_impl_->ResourceHistoryToJson(virtual_address, out_json);
        }
        else
        {
            //This function should not be called if json output is not requested.
            assert(false);
        }
    }
    return result;
}

bool RgdResourceInfoSerializer::GetVirtualAddressHistoryInfo(const Config& user_config, const uint64_t virtual_address, std::string& resource_info_text)
{
    bool result = false;
    if (resource_info_serializer_impl_->va_info_map_.find(virtual_address) == resource_info_serializer_impl_->va_info_map_.end())
    {
        // Resource history for the VA is not built. Build resource history.
        result = resource_info_serializer_impl_->BuildResourceHistoryforVa(user_config, virtual_address);
    }
    else
    {
        // Resource history for the VA is built already.
        result = true;
    }

    if (result)
    {
        if (resource_info_serializer_impl_->IsTextRequired())
        {
            // VA timeline not available for 'all-resources' option.
            if (user_config.is_va_timeline && virtual_address != kVaReserved)
            {

                // Add virtual address residency timeline to output string.
                std::stringstream txt;
                txt << "VA timeline" << std::endl;
                txt << "===========" << std::endl;
                txt << resource_info_serializer_impl_->va_info_map_[virtual_address]->va_residency_info_ << std::endl;
                resource_info_text = txt.str();
            }

            // Add resource create, bind and destroy events timeline.
            std::string resource_timeline;
            resource_info_serializer_impl_->GenerateResourceTimeline(virtual_address, resource_timeline);
            resource_info_text += resource_timeline;

            // Add resource history events info to output string.
            resource_info_serializer_impl_->ResourceHistoryToString(virtual_address, resource_info_text);
        }
        else
        {
            // This function should not be called if text output is not requested.
            assert(false);
        }
    }
    return result;
}

// Build resource map and virtual address residency info string and/or json from history events.
bool RgdResourceInfoSerializer::pImplResourceInfoSerializer::BuildResourceMapFromEvent(const RmtGpuAddress virtual_address, const RmtMemoryEventHistoryHandle     history_handle,
    const RmtMemoryEventHistoryEventIndex event_index)
{
    RmtErrorCode rmt_result = kRmtOk;
    bool result = true;
    RmtResourceHistoryEventType event_type;
    RmtMemoryEventHistoryGetEventType(history_handle, event_index, &event_type);

    const char* kTimestampStr = "Timestamp: ";
    const char* kJsonElemTimestamp = "timestamp";
    const char* kJsonElemEvent = "event";
    const char* kJsonElemVirtualAddress = "virtual_address";

    // Get event time stamp
    uint64_t event_timestamp;

    rmt_result = RmtMemoryEventHistoryGetEventTimestamp(history_handle, event_index, &event_timestamp);

    if (rmt_result != kRmtOk)
    {
        result = false;
    }

    std::stringstream txt;

    if (va_info_map_.find(virtual_address) == va_info_map_.end())
    {
        result = false;
        assert(false);
    }
    else
    {
        assert(va_info_map_[virtual_address] != nullptr);
        RgdVaInfo& va_info = *va_info_map_[virtual_address];

        switch (event_type)
        {
        case RmtResourceHistoryEventType::kRmtResourceHistoryEventResourceCreated:
        {
            const RmtMemoryEventHistoryResourceCreateEventInfo* event_info = nullptr;
            if (RmtMemoryEventHistoryGetResourceCreateEventInfo(history_handle, event_index, &event_info) == kRmtOk)
            {
                va_info.resource_map_[event_info->resource_identifier] = std::make_shared<RgdResource>();
                assert(va_info.resource_map_[event_info->resource_identifier] != nullptr);
                if (va_info.resource_map_[event_info->resource_identifier] != nullptr)
                {
                    RgdResource& rgd_resource = *va_info.resource_map_[event_info->resource_identifier];
                    va_info.resource_list_.push_back(va_info.resource_map_[event_info->resource_identifier]);

                    if (event_info->name != nullptr)
                    {
                        rgd_resource.resource_name.assign(event_info->name);
                    }
                    else
                    {
                        rgd_resource.resource_name = kNullStr;
                    }
                    rgd_resource.rmv_identifier = event_info->resource_identifier;
                    rgd_resource.commit_type = event_info->commit_type;
                    rgd_resource.resource_type = event_info->resource_type;

                    switch (event_info->resource_type)
                    {
                    case kRmtResourceTypeBuffer:
                        memcpy(&rgd_resource.buffer,
                            &event_info->buffer,
                            sizeof(rgd_resource.buffer));
                        break;

                    case kRmtResourceTypeImage:
                        memcpy(&rgd_resource.image,
                            &event_info->image,
                            sizeof(rgd_resource.image));
                        break;

                    case kRmtResourceTypePipeline:
                        memcpy(&rgd_resource.pipeline,
                            &event_info->pipeline,
                            sizeof(rgd_resource.pipeline));
                        break;

                    case kRmtResourceTypeCommandAllocator:
                        memcpy(&rgd_resource.command_allocator,
                            &event_info->command_allocator,
                            sizeof(rgd_resource.command_allocator));
                        break;

                    case kRmtResourceTypeGpuEvent:
                        memcpy(&rgd_resource.gpu_event,
                            &event_info->gpu_event,
                            sizeof(rgd_resource.gpu_event));
                        break;

                    default:
                        break;
                    }

                    // Track resource create, bind or destroy event timeline for this va.
                    va_info.rgd_resource_timeline_.emplace_back(RgdResourceEventType::kCreate,
                        event_info->resource_identifier,
                        event_timestamp);

                    rgd_resource.timeline_indices.push_back(va_info.rgd_resource_timeline_.size() - 1);
                }
            }
            break;
        }
        case RmtResourceHistoryEventType::kRmtResourceHistoryEventResourceBound:
        {
            const RmtMemoryEventHistoryResourceBindEventInfo* event_info = nullptr;
            if (RmtMemoryEventHistoryGetResourceBindEventInfo(history_handle, event_index, &event_info) == kRmtOk)
            {
                assert(va_info.resource_map_[event_info->resource_identifier] != nullptr);
                if(va_info.resource_map_[event_info->resource_identifier] != nullptr)
                {
                    RgdResource& rgd_resource = *va_info.resource_map_[event_info->resource_identifier];
                    rgd_resource.address = event_info->virtual_address;
                    rgd_resource.size_in_bytes = event_info->size_in_bytes;
                    rgd_resource.allocation_base_address = event_info->resource_bound_allocation;
                    rgd_resource.allocation_offset = GetAllocationOffset(event_info->resource_bound_allocation, event_info->virtual_address);

                    // Track resource create, bind or destroy event timeline for this va.
                    va_info.rgd_resource_timeline_.emplace_back(RgdResourceEventType::kBind,
                        event_info->resource_identifier,
                        event_timestamp);

                    // Heap resource type may have multiple bind events.
                    // Keep track of the virtual address, parent allocation base address and size associated with each bind event.
                    assert(&va_info.rgd_resource_timeline_.back() != nullptr);
                    if (&va_info.rgd_resource_timeline_.back() != nullptr)
                    {
                        va_info.rgd_resource_timeline_.back().bound_virtual_address = event_info->virtual_address;
                        va_info.rgd_resource_timeline_.back().bound_base_address = event_info->resource_bound_allocation;
                        va_info.rgd_resource_timeline_.back().bound_size_in_bytes = event_info->size_in_bytes;
                        memcpy(va_info.rgd_resource_timeline_.back().heap_preferences, event_info->heap_preferences, sizeof(va_info.rgd_resource_timeline_.back().heap_preferences));
                    }

                    rgd_resource.timeline_indices.push_back(va_info.rgd_resource_timeline_.size() - 1);
                }
            }
            break;
        }
        case RmtResourceHistoryEventType::kRmtResourceHistoryEventResourceDestroyed:
        {
            const RmtMemoryEventHistoryResourceDestroyEventInfo* event_info = nullptr;
            if (RmtMemoryEventHistoryGetResourceDestroyEventInfo(history_handle, event_index, &event_info) == kRmtOk)
            {
                assert(va_info.resource_map_[event_info->resource_identifier] != nullptr);
                if (va_info.resource_map_[event_info->resource_identifier] != nullptr)
                {
                    RgdResource& rgd_resource = *va_info.resource_map_[event_info->resource_identifier];
                    rgd_resource.destroyed_time = event_timestamp;

                    // Track resource create, bind or destroy event timeline for this va.
                    va_info.rgd_resource_timeline_.emplace_back(RgdResourceEventType::kDestroy,
                        event_info->resource_identifier,
                        event_timestamp);

                    rgd_resource.timeline_indices.push_back(va_info.rgd_resource_timeline_.size() - 1);
                }
            }
            break;
        }
        case RmtResourceHistoryEventType::kRmtResourceHistoryEventVirtualMemoryMapped:
        {
            const RmtMemoryEventHistoryVirtualMemoryMapEventInfo* event_info = nullptr;
            if (virtual_address != kVaReserved && RmtMemoryEventHistoryGetVirtualMemoryMapEventInfo(history_handle, event_index, &event_info) == kRmtOk)
            {
                if (is_text_required_)
                {
                    txt << ToAddressString(event_info->virtual_address);
                    txt << kTimestampStr << GetTimestampString(event_timestamp) << ": Mapped" << std::endl;
                }
                if (is_json_required_)
                {
                    va_info.va_residency_json_.push_back({
                        {kJsonElemVirtualAddress, event_info->virtual_address },
                        {kJsonElemEvent, "mapped"},
                        {kJsonElemTimestamp, event_timestamp}
                        });
                }
            }
            break;
        }
        case RmtResourceHistoryEventType::kRmtResourceHistoryEventVirtualMemoryUnmapped:
        {
            const RmtMemoryEventHistoryVirtualMemoryMapEventInfo* event_info = nullptr;
            if (virtual_address != kVaReserved && RmtMemoryEventHistoryGetVirtualMemoryUnmapEventInfo(history_handle, event_index, &event_info) == kRmtOk)
            {
                if (is_text_required_)
                {
                    txt << ToAddressString(event_info->virtual_address);
                    txt << kTimestampStr << GetTimestampString(event_timestamp) << ": Unmapped" << std::endl;
                }
                if (is_json_required_)
                {
                    va_info.va_residency_json_.push_back({
                        {kJsonElemVirtualAddress, event_info->virtual_address },
                        {kJsonElemEvent, "unmapped"},
                        {kJsonElemTimestamp, event_timestamp}
                        });
                }
            }
            break;
        }
        case RmtResourceHistoryEventType::kRmtResourceHistoryEventVirtualMemoryAllocated:
        {
            const RmtMemoryEventHistoryVirtualMemoryAllocationEventInfo* event_info = nullptr;
            if (virtual_address != kVaReserved && RmtMemoryEventHistoryGetVirtualMemoryAllocationEventInfo(history_handle, event_index, &event_info) == kRmtOk)
            {
                if (is_text_required_)
                {
                    txt << ToAddressString(event_info->virtual_address);
                    txt << kTimestampStr << GetTimestampString(event_timestamp) << ": Allocated";
                    txt << "\t\tPreferred heap: " << ToPreferredHeapString(event_info->preference[0]) << std::endl;
                }
                if (is_json_required_)
                {
                    va_info.va_residency_json_.push_back({
                        {kJsonElemVirtualAddress, event_info->virtual_address },
                        {kJsonElemEvent, "allocated"},
                        {kJsonElemTimestamp, event_timestamp},
                        {"preferred_heap", ToPreferredHeapString(event_info->preference[0])}
                        });
                }
            }
            break;
        }

        case RmtResourceHistoryEventType::kRmtResourceHistoryEventVirtualMemoryFree:
        {
            const RmtMemoryEventHistoryVirtualMemoryFreeEventInfo* event_info = nullptr;
            if (virtual_address != kVaReserved && RmtMemoryEventHistoryGetVirtualMemoryFreeEventInfo(history_handle, event_index, &event_info) == kRmtOk)
            {
                if (is_text_required_)
                {
                    txt << ToAddressString(event_info->virtual_address);
                    txt << kTimestampStr << GetTimestampString(event_timestamp) << ": Free" << std::endl;
                }
                if (is_json_required_)
                {
                    va_info.va_residency_json_.push_back({
                        {kJsonElemVirtualAddress, event_info->virtual_address },
                        {kJsonElemEvent, "free"},
                        {kJsonElemTimestamp, event_timestamp}
                        });
                }
            }
            break;
        }
        case RmtResourceHistoryEventType::kRmtResourceHistoryEventVirtualMemoryMakeResident:
        {
            const RmtMemoryEventHistoryVirtualMemoryResidentEventInfo* event_info = nullptr;
            if (RmtMemoryEventHistoryGetVirtualMemoryMakeResidentEventInfo(history_handle, event_index, &event_info) == kRmtOk)
            {
                // VA Timeline is not built for '--all-resources' option (When virtual address is set to zero).
                if (virtual_address != kVaReserved)
                {
                    if (is_text_required_)
                    {
                        txt << ToAddressString(event_info->virtual_address);
                        txt << kTimestampStr << GetTimestampString(event_timestamp) << ": Make resident" << std::endl;
                    }
                    if (is_json_required_)
                    {
                        va_info.va_residency_json_.push_back({
                            {kJsonElemVirtualAddress, event_info->virtual_address },
                            {kJsonElemEvent, "make_resident"},
                            {kJsonElemTimestamp, event_timestamp}
                            });
                    }
                }

                // Iterate through the associated resources array and add Evict event for each resource id.
                for (size_t array_index = 0; array_index < event_info->resource_count; array_index++)
                {
                    RmtResourceIdentifier current_resource_id = event_info->resource_identifier_array[array_index];

                    assert(va_info.resource_map_.find(current_resource_id) != va_info.resource_map_.end());
                    assert(va_info.resource_map_[current_resource_id] != nullptr);
                    if (va_info.resource_map_.find(current_resource_id) != va_info.resource_map_.end() && va_info.resource_map_[current_resource_id] != nullptr)
                    {
                        RgdResource& current_rgd_resource = *va_info.resource_map_[current_resource_id];

                        // Do not add resource timeline event if resource is destroyed already.
                        assert(current_rgd_resource.destroyed_time == 0 || current_rgd_resource.destroyed_time > event_timestamp);
                        if (current_rgd_resource.destroyed_time == 0 || current_rgd_resource.destroyed_time > event_timestamp)
                        {
                            // Track resource create, bind, make resident, evict or destroy event timeline for this va.
                            va_info.rgd_resource_timeline_.emplace_back(RgdResourceEventType::kMakeResident,
                                current_resource_id,
                                event_timestamp);
                            assert(&va_info.rgd_resource_timeline_.back() != nullptr);
                            if (&va_info.rgd_resource_timeline_.back() != nullptr)
                            {
                                // Keep track of made resident virtual address.
                                va_info.rgd_resource_timeline_.back().bound_virtual_address = event_info->virtual_address;
                            }
                            current_rgd_resource.timeline_indices.push_back(va_info.rgd_resource_timeline_.size() - 1);
                        }
                    }
                }
            }
            break;
        }
        case RmtResourceHistoryEventType::kRmtResourceHistoryEventVirtualMemoryEvict:
        {
            const RmtMemoryEventHistoryVirtualMemoryEvictEventInfo* event_info = nullptr;
            if (RmtMemoryEventHistoryGetVirtualMemoryEvictEventInfo(history_handle, event_index, &event_info) == kRmtOk)
            {
                // VA Timeline is not built for '--all-resources' option (When virtual address is set to zero).
                if (virtual_address != kVaReserved)
                {
                    if (is_text_required_)
                    {
                        txt << ToAddressString(event_info->virtual_address);
                        txt << kTimestampStr << GetTimestampString(event_timestamp) << ": Evict" << std::endl;
                    }
                    if (is_json_required_)
                    {
                        va_info.va_residency_json_.push_back({
                            {kJsonElemVirtualAddress, event_info->virtual_address },
                            {kJsonElemEvent, "evict"},
                            {kJsonElemTimestamp, event_timestamp}
                            });
                    }
                }

                // Iterate through the associated resources array and add Evict event for each resource id.
                for (size_t array_index = 0; array_index < event_info->resource_count; array_index++)
                {
                    RmtResourceIdentifier current_resource_id = event_info->resource_identifier_array[array_index];

                    assert(va_info.resource_map_.find(current_resource_id) != va_info.resource_map_.end());
                    assert(va_info.resource_map_[current_resource_id] != nullptr);
                    if (va_info.resource_map_.find(current_resource_id) != va_info.resource_map_.end() && va_info.resource_map_[current_resource_id] != nullptr)
                    {
                        RgdResource& current_rgd_resource = *va_info.resource_map_[current_resource_id];

                        // Do not add resource timeline event if resource is destroyed already.
                        assert(current_rgd_resource.destroyed_time == 0 || current_rgd_resource.destroyed_time > event_timestamp);
                        if (current_rgd_resource.destroyed_time == 0 || current_rgd_resource.destroyed_time > event_timestamp)
                        {
                            // Track resource create, bind, make resident, evict or destroy event timeline for this va.
                            va_info.rgd_resource_timeline_.emplace_back(RgdResourceEventType::kEvict,
                                current_resource_id,
                                event_timestamp);

                            current_rgd_resource.timeline_indices.push_back(va_info.rgd_resource_timeline_.size() - 1);
                        }
                    }
                }
            }
            break;
        }
        default:
            break;
        }

        if (virtual_address != kVaReserved && is_text_required_)
        {
            va_info.va_residency_info_ += txt.str();
        }
    }
    return result;
}

uint64_t RgdResourceInfoSerializer::pImplResourceInfoSerializer::GetResourceCreateTime(const RgdVaInfo& va_info, const RgdResource& rgd_resource) const
{
    uint64_t resource_create_time = 0;

    // Find create event and return create time.
    for (size_t i = 0; i < rgd_resource.timeline_indices.size(); ++i)
    {
        const size_t resource_timeline_idx = rgd_resource.timeline_indices[i];
        assert(resource_timeline_idx < va_info.rgd_resource_timeline_.size());
        assert(&va_info.rgd_resource_timeline_[resource_timeline_idx] != nullptr);
        const RgdResourceTimeline& resource_timeline_event = va_info.rgd_resource_timeline_[resource_timeline_idx];

        if (resource_timeline_event.event_type == RgdResourceEventType::kCreate)
        {
            resource_create_time = resource_timeline_event.event_timestamp;
            break;
        }
    }

    return resource_create_time;
}

void RgdResourceInfoSerializer::pImplResourceInfoSerializer::FindAndUpdateRmtResourceAndImplicitHeapPair(const uint64_t virtual_address)
{
    const size_t kImplicitResourceCheckClockTicksThreshold = 2;
    assert(va_info_map_.find(virtual_address) != va_info_map_.end());
    if (va_info_map_.find(virtual_address) != va_info_map_.end())
    {
        const RgdVaInfo& va_info = *va_info_map_[virtual_address];
        for (size_t resource_idx = 0; resource_idx < va_info.resource_list_.size(); ++resource_idx)
        {
            assert(va_info.resource_list_[resource_idx] != nullptr);
            if (va_info.resource_list_[resource_idx] != nullptr)
            {
                RgdResource& current_resource = *va_info.resource_list_[resource_idx];

                for (size_t i = resource_idx + 1; i < va_info.resource_list_.size(); ++i)
                {
                    assert(va_info.resource_list_[i] != nullptr);
                    if (va_info.resource_list_[i] != nullptr)
                    {
                        RgdResource& next_resource = *va_info.resource_list_[i];

                        // Check for next resources until creation time of the next resource is within kImplicitResourceCheckClockTicksThreshold clock ticks.
                        if (GetResourceCreateTime(va_info, next_resource) > (kImplicitResourceCheckClockTicksThreshold + GetResourceCreateTime(va_info, current_resource)))
                        {
                            break;
                        }

                        // Check if only one of the two resources being compared is 'Heap' resource.
                        if ((current_resource.resource_type == kRmtResourceTypeHeap) != (next_resource.resource_type == kRmtResourceTypeHeap))
                        {
                            // If 'Heap' resource found, check if both resources have valid associated_resource_idx and they are same in size and their virtual address and base allocation address matches with each other.
                            if (current_resource.associated_resource_idx == -1
                                && next_resource.associated_resource_idx == -1
                                && next_resource.size_in_bytes == current_resource.size_in_bytes
                                && next_resource.address == current_resource.address
                                && next_resource.allocation_base_address == current_resource.allocation_base_address)
                            {
                                // Associated resource found.
                                current_resource.associated_resource_idx = i;
                                next_resource.associated_resource_idx = resource_idx;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

// Format resource history as a string.
void RgdResourceInfoSerializer::pImplResourceInfoSerializer::ResourceHistoryToString(const uint64_t virtual_address, std::string& out_text)
{
    std::stringstream txt;
    const char* kResourceTypeStr = "Type: ";
    const char* kResourceAttributeStr = "Attributes";
    const char* kResourceIdStr = "Resource id: ";
    const char* kNameStr = "Name: ";

    // update txt with va residency info.
    txt << out_text;

    txt << "Associated resources" << std::endl;
    txt << "====================" << std::endl;

    const RgdVaInfo& va_info = *va_info_map_[virtual_address];

    if (va_info_map_.find(virtual_address) != va_info_map_.end())
    {
        if (va_info.resource_map_.empty())
        {
            txt << "INFO: no associated resources detected for the offending VA.";
        }
    }
    else
    {
        //virtual address not present.
        assert(false);
    }

    for(size_t i = 0; i < va_info.resource_list_.size(); ++i)
    {
        const RgdResource& current_resource = *va_info.resource_list_[i];

        // Upon creation of a committed resource an implicit heap is created or upon a creation of a heap an implicit buffer is created.
        // This pair of committed resource and associated implicit resource will be consolidated and treated as one resource for default RGD output.
        // Skip consolidating implicit resources if '--expand-implicit-resources' option is not used.
        // associated_resource_idx holds the index of the associated RmtResource and implicit resource pair.
        // If '--expand-implicit-resource' option is used, associated_resource_idx will not be updated.
        bool is_implicit_resource = (current_resource.associated_resource_idx != -1 && current_resource.resource_type == kRmtResourceTypeHeap);
        if (!is_implicit_resource)
        {
            // Does this resource has associated implicit resource or does the '--expand-implicit-resources' option is used?
            bool is_resource_pair_print = false;
            if (current_resource.associated_resource_idx != -1)
            {
                // This resource has associated implicit resource. Serialize the information of both resources together.
                assert(va_info.resource_list_[current_resource.associated_resource_idx] != nullptr);
                if (va_info.resource_list_[current_resource.associated_resource_idx] != nullptr)
                {
                    is_resource_pair_print = true;
                }
            }

            if (is_resource_pair_print)
            {
                const RgdResource& associated_resource = *va_info.resource_list_[current_resource.associated_resource_idx];

                // The resource at associated_resource_index should be a 'Heap' or a 'Buffer'.
                assert(associated_resource.resource_type == kRmtResourceTypeHeap || associated_resource.resource_type == kRmtResourceTypeBuffer);

                txt << kResourceIdStr << std::hex << "<0x" << current_resource.rmv_identifier << ", 0x" << associated_resource.rmv_identifier << ">" << std::dec << std::endl;

                // Print resource type of both resources, Example: "Type: <Heap, Image>"
                txt << "\t" << kResourceTypeStr << "<" << GetResourceTypeText(kRmtResourceTypeHeap) << ", " << GetResourceTypeText(current_resource.resource_type) << ">" << std::endl;
                txt << "\t" << kNameStr << ((current_resource.resource_name != kNullStr) ? current_resource.resource_name : associated_resource.resource_name) << std::endl;
            }
            else
            {
                // This resource is not associated with an implicit resource or '--expand-implicit-resources' option is used.
                txt << kResourceIdStr << std::hex << "0x" << current_resource.rmv_identifier << std::dec << std::endl;
                txt << "\t" << kResourceTypeStr << GetResourceTypeText(current_resource.resource_type) << std::endl;
                txt << "\t" << kNameStr << current_resource.resource_name << std::endl;
            }

            txt << "\tVirtual address:" << std::endl;

            size_t bind_events_count = 0;

            // Iterate through all the bind events and print all the virtual addresses.
            for (size_t index = 0; index < current_resource.timeline_indices.size(); index++)
            {
                const size_t resource_timeline_idx = current_resource.timeline_indices[index];
                assert(resource_timeline_idx < va_info.rgd_resource_timeline_.size());
                assert(&va_info.rgd_resource_timeline_[resource_timeline_idx] != nullptr);
                const RgdResourceTimeline& resource_timeline_event = va_info.rgd_resource_timeline_[resource_timeline_idx];

                if (resource_timeline_event.event_type == RgdResourceEventType::kBind)
                {
                    bind_events_count++;

                    txt << "\t\t 0x" << std::hex << resource_timeline_event.bound_virtual_address
                        << " [size: " << RgdParsingUtils::GetFormattedSizeString(resource_timeline_event.bound_size_in_bytes)
                        << ", parent address + offset: 0x" << resource_timeline_event.bound_base_address << " + 0x" << GetAllocationOffset(resource_timeline_event.bound_base_address, resource_timeline_event.bound_virtual_address)
                        << ", preferred heap: " << ToPreferredHeapString(resource_timeline_event.heap_preferences[0])
                        << "]" << std::dec;

                    if (resource_timeline_event.bound_virtual_address == virtual_address && bind_events_count > 1)
                    {
                        // For a resource with multiple bind events, point to a bind event which is specific to offending VA.
                        txt << "  <-- Offending VA";
                    }

                    txt << std::endl;
                }
            }

            txt << "\tCommit type: " << GetCommitTypeStringFromCommitType(current_resource.commit_type) << std::endl;

            // Resource Attributes.
            switch (current_resource.resource_type)
            {
            case kRmtResourceTypeBuffer:
                txt << "\t" << kResourceAttributeStr << " (" << kResourceTypeBufferStr << "):" << std::endl;
                txt << "\t\tCreate flags: " << GetFlagsString(current_resource.resource_type, current_resource.buffer.create_flags, "create") << std::endl;
                txt << "\t\tUsage flags: " << GetFlagsString(current_resource.resource_type, current_resource.buffer.usage_flags, "usage") << std::endl;
                break;

            case kRmtResourceTypeImage:
                txt << "\t" << kResourceAttributeStr << " (" << kResourceTypeImageStr << "):" << std::endl;
                txt << "\t\tCreate flags: " << GetFlagsString(current_resource.resource_type, current_resource.image.create_flags, "create") << std::endl;
                txt << "\t\tUsage flags: " << GetFlagsString(current_resource.resource_type, current_resource.image.usage_flags, "usage") << std::endl;
                txt << "\t\tImage type: " << GetImageTypeStringFromImageType(current_resource.image.image_type) << std::endl;
                txt << "\t\tDimensions <x, y, z>: " << current_resource.image.dimension_x << " x " << current_resource.image.dimension_y << " x " << current_resource.image.dimension_z << std::endl;
                txt << "\t\tSwizzle pattern: " << GetSwizzlePatternString(&current_resource.image.format) << std::endl;
                txt << "\t\tImage format: " << GetImageFormatString(current_resource.image.format.format) << std::endl;
                txt << "\t\tMip levels: " << current_resource.image.mip_levels << std::endl;
                txt << "\t\tSlices: " << current_resource.image.slices << std::endl;
                txt << "\t\tSample count: " << current_resource.image.sample_count << std::endl;
                txt << "\t\tFragment count: " << current_resource.image.fragment_count << std::endl;
                txt << "\t\tTiling type: " << GetImageTypeStringFromImageType(current_resource.image.tiling_type) << std::endl;
                break;

            case kRmtResourceTypePipeline:
                txt << "\t" << kResourceAttributeStr << " (" << kResourceTypePipelineStr << "):" << std::endl;
                txt << "\t" << kResourceTypeStr << kResourceTypePipelineStr << std::endl;
                txt << "\t\tCreate flags: " << GetFlagsString(current_resource.resource_type, current_resource.pipeline.create_flags) << std::endl;
                txt << "\t\tInternal pipeline hash - High: 0x" << std::hex << current_resource.pipeline.internal_pipeline_hash_hi << " Low : 0x" << current_resource.pipeline.internal_pipeline_hash_lo << std::dec << std::endl;
                txt << "\t\tStage mask: 0x" << std::hex << current_resource.pipeline.stage_mask << std::dec << std::endl;
                txt << "\t\tIs NGG: " << std::boolalpha << current_resource.pipeline.is_ngg << std::noboolalpha << std::endl;
                break;

            case kRmtResourceTypeCommandAllocator:
                txt << "\t" << kResourceAttributeStr << " (" << kResourceTypeCommandBufferStr << "):" << std::endl;
                txt << "\t" << kResourceTypeStr << kResourceTypeCommandBufferStr << std::endl;
                txt << "\t\tFlags: " << GetFlagsString(current_resource.resource_type, current_resource.command_allocator.flags) << std::endl;
                txt << "\t\tExecutable preferred heap: " << ToPreferredHeapString(current_resource.command_allocator.cmd_data_heap) << std::endl;
                txt << "\t\tExecutable size: " << RgdParsingUtils::GetFormattedSizeString(current_resource.command_allocator.cmd_data_size) << std::endl;
                txt << "\t\tExecutable suballoc size: " << RgdParsingUtils::GetFormattedSizeString(current_resource.command_allocator.cmd_data_suballoc_size) << std::endl;
                txt << "\t\tEmbedded preferred heap: " << ToPreferredHeapString(current_resource.command_allocator.embed_data_heap) << std::endl;
                txt << "\t\tEmbedded size: " << RgdParsingUtils::GetFormattedSizeString(current_resource.command_allocator.embed_data_size) << std::endl;
                txt << "\t\tEmbedded suballoc size: " << RgdParsingUtils::GetFormattedSizeString(current_resource.command_allocator.embed_data_suballoc_size) << std::endl;
                txt << "\t\tGPU scratch preferred heap: " << ToPreferredHeapString(current_resource.command_allocator.gpu_scratch_heap) << std::endl;
                txt << "\t\tGPU scratch size: " << RgdParsingUtils::GetFormattedSizeString(current_resource.command_allocator.gpu_scratch_size) << std::endl;
                txt << "\t\tGPU scratch suballoc size: " << RgdParsingUtils::GetFormattedSizeString(current_resource.command_allocator.gpu_scratch_suballoc_size) << std::endl;
                break;

            case kRmtResourceTypeHeap:
                break;

            case kRmtResourceTypeDescriptorHeap:
            case kRmtResourceTypeDescriptorPool:
                break;

            case kRmtResourceTypeGpuEvent:
                txt << "\t" << kResourceAttributeStr << " (" << kResourceTypeGpuEventStr << "):" << std::endl;
                txt << "\t\tFlags: " << GetFlagsString(current_resource.resource_type, current_resource.gpu_event.flags) << std::endl;
                break;

            case kRmtResourceTypeBorderColorPalette:
            case kRmtResourceTypeTimestamp:
            case kRmtResourceTypeMiscInternal:
            case kRmtResourceTypePerfExperiment:
            case kRmtResourceTypeMotionEstimator:
            case kRmtResourceTypeVideoDecoder:
            case kRmtResourceTypeVideoEncoder:
            case kRmtResourceTypeQueryHeap:
            case kRmtResourceTypeIndirectCmdGenerator:
                break;

            default:
                break;
            }
            txt << "\tResource timeline:" << std::endl;

            // Print Resource Timeline.
            //
            // Build look ahead counter that holds the max count of upcoming events of same type for the respective RgdResourceTimeline event (matched by the index).
            // This is to collapse repeated events and print the timestamp of first and last event.
            //
            // Example:
            // rgd_resource_timeline = {create, bind, Make Resident, Evict, Make Resident, Evict, Make Resident, Make Resident, Make Resident, Make Resident}
            // look_ahead_counter    = {     0,    0,             0,     0,             0,     0,             3,             2,             1,             0}
            //
            // For 3 or More consecutive events of same type, timestamp of the first and last occurrence will be printed.
            std::vector<size_t> look_ahead_counter(current_resource.timeline_indices.size(), 0);

            // Do we have repeated events for a table?
            bool is_repeated_events = false;

            for (intmax_t m = static_cast<intmax_t>(look_ahead_counter.size()) - 1; m > 0; m--)
            {
                const size_t current_idx = current_resource.timeline_indices[m];
                const size_t previous_idx = current_resource.timeline_indices[m - 1];
                assert(&va_info.rgd_resource_timeline_[current_idx] != nullptr && &va_info.rgd_resource_timeline_[previous_idx] != nullptr);
                const RgdResourceTimeline& current_event = va_info.rgd_resource_timeline_[current_idx];
                const RgdResourceTimeline& previous_event = va_info.rgd_resource_timeline_[previous_idx];
                if (is_repeated_events == false
                    && current_event.event_type == previous_event.event_type
                    && current_event.bound_base_address == previous_event.bound_base_address)
                {
                    look_ahead_counter[m - 1] = look_ahead_counter[m] + 1;
                    is_repeated_events = (look_ahead_counter[m - 1] > 1) ? true : false;
                }
            }

            int timestamp_width = kTimestampWidth;
            if (is_repeated_events)
            {
                timestamp_width = kTimestampWidth + kTimestampWidth + 1;
            }

            // holds the max count of same type of upcoming events.
            size_t same_events_count = 0;

            std::stringstream timestamp_txt;

            for (size_t index = 0; index < current_resource.timeline_indices.size(); index++)
            {
                const size_t resource_timeline_idx = current_resource.timeline_indices[index];
                assert(resource_timeline_idx < va_info.rgd_resource_timeline_.size());
                assert(&va_info.rgd_resource_timeline_[resource_timeline_idx] != nullptr);
                const RgdResourceTimeline& resource_timeline_event = va_info.rgd_resource_timeline_[resource_timeline_idx];

                if (index == 0 || look_ahead_counter[index - 1] == 0 || same_events_count == 0)
                {
                    // Set same events count.
                    same_events_count = look_ahead_counter[index];
                    txt << "\t\t";
                    timestamp_txt << GetTimestampString(resource_timeline_event.event_timestamp);
                }

                if (look_ahead_counter[index] == 0 || same_events_count < kMinEventsToExpand)
                {
                    if (same_events_count >= kMinEventsToExpand)
                    {
                        timestamp_txt << ".." << GetTimestampString(resource_timeline_event.event_timestamp);
                    }
                    PrintFormattedResourceTimeline(timestamp_txt.str(), timestamp_width, txt);

                    // Clear the timestamp txt stream.
                    timestamp_txt.str(std::string());

                    switch (resource_timeline_event.event_type)
                    {
                    case RgdResourceEventType::kCreate:
                        txt << ": " << kResourceEventTypeCreateStr;
                        break;

                    case RgdResourceEventType::kBind:
                        txt << ": " << kResourceEventTypeBindStr << " into 0x" << std::hex << resource_timeline_event.bound_virtual_address << std::dec;
                        break;

                    case RgdResourceEventType::kMakeResident:
                        txt << ": " << kResourceEventTypeMakeResidentStr << " into 0x" << std::hex << resource_timeline_event.bound_virtual_address << std::dec;
                        break;

                    case RgdResourceEventType::kEvict:
                        txt << ": " << kResourceEventTypeEvictStr;
                        break;

                    case RgdResourceEventType::kDestroy:
                        txt << ": " << kResourceEventTypeDestroy;
                        break;

                    default:
                        // Should not reach here.
                        assert(false);
                    }

                    if (same_events_count >= kMinEventsToExpand)
                    {
                        txt << " (" << same_events_count + 1 << " occurrences)";
                    }
                    txt << std::endl;

                    same_events_count = 0;
                }
            }

            txt << std::endl;
        }
    }
    txt << std::endl;
    out_text = txt.str();
}

//Format resource history as a json.
bool RgdResourceInfoSerializer::pImplResourceInfoSerializer::ResourceHistoryToJson(const uint64_t virtual_address, nlohmann::json& resource_info_json)
{
    bool ret = true;

    const char* kOffendingVaResources = "resource_information";
    const char* kResourceTimeline = "resource_timeline";
    const char* kResourceType = "resource_type";
    const char* kResourceTypeBuffer = "buffer";
    const char* kResourceTypeImage = "image";
    const char* kResourceTypePipeline = "pipeline";
    const char* kResourceTypeCommandAllocator = "command_buffer";
    const char* kResourceTypeHeap = "heap";
    const char* kResourceTypeDescriptor = "descriptor";
    const char* kResourceTypeGpuEvent = "gpu_event";
    const char* kResourceTypeInternal = "internal";
    const char* kResourceCreateFlags = "create_flags";
    const char* kResourceUsageFlags = "usage_flags";
    const char* kResourceFlags = "flags";

    assert(va_info_map_[virtual_address] != nullptr);
    if (va_info_map_[virtual_address] != nullptr)
    {
        RgdVaInfo& va_info = *va_info_map_[virtual_address];

        try
        {
            resource_info_json[kOffendingVaResources] = nlohmann::json::array();
            resource_info_json[kJsonElemPageFaultSummary]["offending_va"] = virtual_address;
            for (size_t i = 0; i < va_info.resource_list_.size(); i++)
            {
                assert(va_info.resource_list_[i] != nullptr);
                if (va_info.resource_list_[i] != nullptr)
                {
                    const RgdResource& rgd_resource = *va_info.resource_list_[i];

                    // Upon creation of a committed resource an implicit heap is created or upon a creation of a heap an implicit buffer is created.
                    // This pair of committed resource and associated implicit resource will be consolidated and treated as one resource for default RGD output.
                    // Skip consolidating implicit resources if '--expand-implicit-resources' option is not used.
                    // associated_resource_idx holds the index of the associated RmtResource and implicit resource pair.
                    // If '--expand-implicit-resource' option is used, associated_resource_idx will not be updated.
                    bool is_implicit_resource = (rgd_resource.associated_resource_idx != -1 && rgd_resource.resource_type == kRmtResourceTypeHeap);

                    if (!is_implicit_resource)
                    {
                        // Does this resource has associated implicit resource or does the '--expand-implicit-resources' option is used?
                        bool is_resource_pair_print = false;
                        if (rgd_resource.associated_resource_idx != -1)
                        {
                            // This resource has associated implicit resource. Serialize the information of both resources together.
                            assert(va_info.resource_list_[rgd_resource.associated_resource_idx] != nullptr);
                            if (va_info.resource_list_[rgd_resource.associated_resource_idx] != nullptr)
                            {
                                is_resource_pair_print = true;
                            }
                        }

                        nlohmann::json resource_element;
                        resource_element["resource_id"] = rgd_resource.rmv_identifier;

                        if (is_resource_pair_print)
                        {
                            const RgdResource& associated_resource = *va_info.resource_list_[rgd_resource.associated_resource_idx];

                            // The resource at associated_resource_index should be a 'Heap' or a 'Buffer'.
                            assert(associated_resource.resource_type == kRmtResourceTypeHeap || associated_resource.resource_type == kRmtResourceTypeBuffer);
                            resource_element["name"] = (rgd_resource.resource_name != kNullStr) ? rgd_resource.resource_name : associated_resource.resource_name;
                            resource_element["associated_resource_id"] = associated_resource.rmv_identifier;
                            resource_element["associated_resource_type"] = GetResourceTypeText(kRmtResourceTypeHeap);
                        }
                        else
                        {
                            resource_element["name"] = rgd_resource.resource_name;
                        }

                        static const char* kBoundVirtualAddressRanges = "bound_virtual_address_ranges";
                        resource_element[kBoundVirtualAddressRanges] = nlohmann::json::array();

                        // Iterate through all the bind events and add all the virtual addresses.
                        for (size_t index = 0; index < rgd_resource.timeline_indices.size(); index++)
                        {
                            const size_t resource_timeline_idx = rgd_resource.timeline_indices[index];
                            assert(resource_timeline_idx < va_info.rgd_resource_timeline_.size());
                            assert(&va_info.rgd_resource_timeline_[resource_timeline_idx] != nullptr);
                            const RgdResourceTimeline& resource_timeline_event = va_info.rgd_resource_timeline_[resource_timeline_idx];

                            if (resource_timeline_event.event_type == RgdResourceEventType::kBind)
                            {
                                resource_element[kBoundVirtualAddressRanges].push_back({
                                    { "bind_event_timestamp", resource_timeline_event.event_timestamp },
                                    { "virtual_address", resource_timeline_event.bound_virtual_address },
                                    { "parent_allocation_base_address", resource_timeline_event.bound_base_address },
                                    { "offset_within_allocation", GetAllocationOffset(resource_timeline_event.bound_base_address, resource_timeline_event.bound_virtual_address) },
                                    { "size_in_bytes", resource_timeline_event.bound_size_in_bytes},
                                    { "preferred_heap", ToPreferredHeapString(resource_timeline_event.heap_preferences[0])}
                                    });
                            }
                        }
                        resource_element["commit_type"] = GetCommitTypeStringFromCommitType(rgd_resource.commit_type);
                        resource_element[kResourceTimeline] = nlohmann::json::array();

                        for (size_t resource_timeline_idx : rgd_resource.timeline_indices)
                        {
                            assert(&va_info.rgd_resource_timeline_[resource_timeline_idx] != nullptr);
                            if (resource_timeline_idx < va_info.rgd_resource_timeline_.size() && &va_info.rgd_resource_timeline_[resource_timeline_idx] != nullptr)
                            {
                                const RgdResourceTimeline& resource_timeline_event = va_info.rgd_resource_timeline_[resource_timeline_idx];

                                switch (resource_timeline_event.event_type)
                                {
                                case RgdResourceEventType::kCreate:
                                    resource_element[kResourceTimeline].push_back({ resource_timeline_event.event_timestamp, "create" });
                                    break;

                                case RgdResourceEventType::kBind:
                                    resource_element[kResourceTimeline].push_back({ resource_timeline_event.event_timestamp, "bind" });
                                    break;

                                case RgdResourceEventType::kMakeResident:
                                    resource_element[kResourceTimeline].push_back({ resource_timeline_event.event_timestamp, "make_resident" });
                                    break;

                                case RgdResourceEventType::kEvict:
                                    resource_element[kResourceTimeline].push_back({ resource_timeline_event.event_timestamp, "evict" });
                                    break;

                                case RgdResourceEventType::kDestroy:
                                    resource_element[kResourceTimeline].push_back({ resource_timeline_event.event_timestamp, "destroy" });
                                    break;

                                default:
                                    // Should not reach here.
                                    assert(false);
                                }
                            }
                            else
                            {
                                // Should not reach here.
                                assert(false);
                            }
                        }

                        switch (rgd_resource.resource_type)
                        {
                        case kRmtResourceTypeBuffer:
                            resource_element[kResourceType][kResourceTypeBuffer][kResourceCreateFlags] = GetFlagsString(rgd_resource.resource_type, rgd_resource.buffer.create_flags, "create");
                            resource_element[kResourceType][kResourceTypeBuffer][kResourceUsageFlags] = GetFlagsString(rgd_resource.resource_type, rgd_resource.buffer.usage_flags, "usage");
                            break;

                        case kRmtResourceTypeImage:
                            resource_element[kResourceType][kResourceTypeImage][kResourceCreateFlags] = GetFlagsString(rgd_resource.resource_type, rgd_resource.image.create_flags, "create");
                            resource_element[kResourceType][kResourceTypeImage][kResourceUsageFlags] = GetFlagsString(rgd_resource.resource_type, rgd_resource.image.usage_flags, "usage");
                            resource_element[kResourceType][kResourceTypeImage]["image_type"] = GetImageTypeStringFromImageType(rgd_resource.image.image_type);
                            resource_element[kResourceType][kResourceTypeImage]["x_dimension"] = rgd_resource.image.dimension_x;
                            resource_element[kResourceType][kResourceTypeImage]["y_dimension"] = rgd_resource.image.dimension_y;
                            resource_element[kResourceType][kResourceTypeImage]["z_dimension"] = rgd_resource.image.dimension_z;
                            resource_element[kResourceType][kResourceTypeImage]["swizzle_pattern"] = GetSwizzlePatternString(&rgd_resource.image.format);
                            resource_element[kResourceType][kResourceTypeImage]["image_format"] = GetImageFormatString(rgd_resource.image.format.format);
                            resource_element[kResourceType][kResourceTypeImage]["mip_levels"] = rgd_resource.image.mip_levels;
                            resource_element[kResourceType][kResourceTypeImage]["slices"] = rgd_resource.image.slices;
                            resource_element[kResourceType][kResourceTypeImage]["sample_count"] = rgd_resource.image.sample_count;
                            resource_element[kResourceType][kResourceTypeImage]["fragment_count"] = rgd_resource.image.fragment_count;
                            resource_element[kResourceType][kResourceTypeImage]["tiling_type"] = GetImageTypeStringFromImageType(rgd_resource.image.tiling_type);
                            break;

                        case kRmtResourceTypePipeline:
                            resource_element[kResourceType][kResourceTypePipeline][kResourceCreateFlags] = GetFlagsString(rgd_resource.resource_type, rgd_resource.pipeline.create_flags, "create");
                            resource_element[kResourceType][kResourceTypePipeline]["internal_pipeline_hash_high"] = rgd_resource.pipeline.internal_pipeline_hash_hi;
                            resource_element[kResourceType][kResourceTypePipeline]["internal_pipeline_hash_low"] = rgd_resource.pipeline.internal_pipeline_hash_lo;
                            resource_element[kResourceType][kResourceTypePipeline]["stage_mask"] = rgd_resource.pipeline.stage_mask;
                            resource_element[kResourceType][kResourceTypePipeline]["is_ngg"] = rgd_resource.pipeline.is_ngg;
                            break;

                        case kRmtResourceTypeCommandAllocator:
                            resource_element[kResourceType][kResourceTypeCommandAllocator][kResourceFlags] = GetFlagsString(rgd_resource.resource_type, rgd_resource.command_allocator.flags);
                            resource_element[kResourceType][kResourceTypeCommandAllocator]["executable_preferred_heap"] = ToPreferredHeapString(rgd_resource.command_allocator.cmd_data_heap);
                            resource_element[kResourceType][kResourceTypeCommandAllocator]["executable_size"] = rgd_resource.command_allocator.cmd_data_size;
                            resource_element[kResourceType][kResourceTypeCommandAllocator]["executable_suballoc_size"] = rgd_resource.command_allocator.cmd_data_suballoc_size;
                            resource_element[kResourceType][kResourceTypeCommandAllocator]["embedded_preferred_heap"] = ToPreferredHeapString(rgd_resource.command_allocator.embed_data_heap);
                            resource_element[kResourceType][kResourceTypeCommandAllocator]["embedded_size"] = rgd_resource.command_allocator.embed_data_size;
                            resource_element[kResourceType][kResourceTypeCommandAllocator]["embedded_suballoc_size"] = rgd_resource.command_allocator.embed_data_suballoc_size;
                            resource_element[kResourceType][kResourceTypeCommandAllocator]["gpu_scratch_preferred_heap"] = ToPreferredHeapString(rgd_resource.command_allocator.gpu_scratch_heap);
                            resource_element[kResourceType][kResourceTypeCommandAllocator]["gpu_scratch_size"] = rgd_resource.command_allocator.gpu_scratch_size;
                            resource_element[kResourceType][kResourceTypeCommandAllocator]["gpu_scratch_suballoc_size"] = rgd_resource.command_allocator.gpu_scratch_suballoc_size;
                            break;

                        case kRmtResourceTypeHeap:
                            resource_element[kResourceType][kResourceTypeHeap] = NULL;
                            break;

                        case kRmtResourceTypeDescriptorHeap:
                        case kRmtResourceTypeDescriptorPool:
                            resource_element[kResourceType][kResourceTypeDescriptor] = NULL;
                            break;

                        case kRmtResourceTypeGpuEvent:
                            resource_element[kResourceType][kResourceTypeGpuEvent][kResourceFlags] = GetFlagsString(rgd_resource.resource_type, rgd_resource.gpu_event.flags);
                            break;

                        case kRmtResourceTypeBorderColorPalette:
                        case kRmtResourceTypeTimestamp:
                        case kRmtResourceTypeMiscInternal:
                        case kRmtResourceTypePerfExperiment:
                        case kRmtResourceTypeMotionEstimator:
                        case kRmtResourceTypeVideoDecoder:
                        case kRmtResourceTypeVideoEncoder:
                        case kRmtResourceTypeQueryHeap:
                        case kRmtResourceTypeIndirectCmdGenerator:
                            resource_element[kResourceType][kResourceTypeInternal] = NULL;
                            break;

                        default:
                            resource_element[kResourceType]["unknown"] = NULL;
                            break;
                        }

                        resource_info_json[kJsonElemPageFaultSummary][kOffendingVaResources].push_back(resource_element);
                    }
                }
            }
        }
        catch (nlohmann::json::exception e)
        {
            ret = false;
            RgdUtils::PrintMessage(e.what(),
                RgdMessageType::kError, true);
        }
    }
    else
    {
        ret = false;
    }
    return ret;
}

// Set output format.
void RgdResourceInfoSerializer::pImplResourceInfoSerializer::SetOutputFormat(const Config& user_config)
{
    // True if the output we are required to produce is in text format (file or console).
    is_text_required_ = !user_config.output_file_txt.empty() || user_config.output_file_json.empty();
    is_json_required_ = !user_config.output_file_json.empty();
    is_raw_time_ = user_config.is_raw_time;
}

void RgdResourceInfoSerializer::pImplResourceInfoSerializer::SetCpuFrequency(const uint64_t cpu_frequency)
{
    assert(cpu_frequency != 0);
    if (cpu_frequency == 0)
    {
        is_raw_time_ = true;
        RgdUtils::PrintMessage(kPrintRawTimestampMsg, RgdMessageType::kWarning, true);
    }
    cpu_frequency_ = cpu_frequency;
}

void RgdResourceInfoSerializer::pImplResourceInfoSerializer::SetTargetProcessId(const uint64_t target_process_id)
{
    target_process_id_ = target_process_id;
}

uint64_t RgdResourceInfoSerializer::pImplResourceInfoSerializer::GetTargetProcessId() const
{
    return target_process_id_;
}

void RgdResourceInfoSerializer::pImplResourceInfoSerializer::GenerateResourceTimeline(const uint64_t virtual_address, std::string& resource_timeline)
{
    const int kEventTypeWidth = 16;
    const int kResourceTypeWidth = 22;
    const int kResourceSizeWidth = 28;
    const int kResourceIdWidth = 43;
    const int kResourceNameWidth = 50;
    const int kRepeatedOccurrenceStrWidth = 42;

    std::stringstream txt;

    txt << "Resource timeline" << std::endl;
    txt << "=================" << std::endl;

    std::stringstream legend_txt;
    legend_txt << std::endl;
    legend_txt << "Legend" << std::endl;
    legend_txt << "======" << std::endl;
    legend_txt << "<> denotes paired resources: in certain cases, a resource (heap or buffer) is created implicitly with another resource by the runtime or the driver. When the tool detects this situation, the two resources will appear as a single pair inside <>" << std::endl;

    std::stringstream timestamp_format_txt;
    timestamp_format_txt << "Timestamp format: HH:MM:SS.clk_cycles (clk_cycles per second: " << RgdUtils::ToFormattedNumericString(cpu_frequency_) << ")" << std::endl;

    if (va_info_map_.find(virtual_address) != va_info_map_.end()
        && !va_info_map_[virtual_address]->rgd_resource_timeline_.empty())
    {
        RgdVaInfo& va_info = *va_info_map_[virtual_address];
        va_info.SortResourceTimeline();

        // Build look ahead counter that holds the max count of upcoming events of same type and same resource id for the respective RgdResourceTimeline event (matched by the index).
        // For 3 or More consecutive events of same type for the same resource id, timestamp of the first and last occurrence will be printed.
        std::vector<size_t> look_ahead_counter(va_info.rgd_resource_timeline_.size(), 0);

        // Do we have repeated events for a table?
        bool is_repeated_events = false;
        for (intmax_t m = static_cast<intmax_t>(look_ahead_counter.size()) - 1; m > 0; m--)
        {
            const size_t current_idx = m;
            const size_t previous_idx = m - 1;
            assert(&va_info.rgd_resource_timeline_[current_idx] != nullptr && &va_info.rgd_resource_timeline_[previous_idx] != nullptr);
            const RgdResourceTimeline& current_event = va_info.rgd_resource_timeline_[current_idx];
            const RgdResourceTimeline& previous_event = va_info.rgd_resource_timeline_[previous_idx];
            if (is_repeated_events == false
                && current_event.event_type == previous_event.event_type
                && current_event.identifier == previous_event.identifier)
            {
                look_ahead_counter[previous_idx] = look_ahead_counter[current_idx] + 1;
                is_repeated_events = (look_ahead_counter[previous_idx] > 1) ? true : false;
            }
        }

        int timestamp_width = kTimestampWidth;
        if (is_repeated_events)
        {
            timestamp_width = kTimestampWidth + kRepeatedOccurrenceStrWidth;
        }

        // holds the max count of same type of upcoming events for the same resource id.
        size_t same_events_count = 0;

        // Print the legend note for '<>' notation only when the resource timeline includes at least one pair of RmtResource and associated implicit resource.
        bool is_print_legend = false;
        std::stringstream timeline_txt;
        std::stringstream timestamp_txt;

        PrintFormattedResourceTimeline("Timestamp", timestamp_width, timeline_txt);
        PrintFormattedResourceTimeline("Event type", kEventTypeWidth, timeline_txt);
        PrintFormattedResourceTimeline("Resource type", kResourceTypeWidth, timeline_txt);
        PrintFormattedResourceTimeline("Resource identifier", kResourceIdWidth, timeline_txt);
        PrintFormattedResourceTimeline("Resource size", kResourceSizeWidth, timeline_txt);
        PrintFormattedResourceTimeline("Resource name", kResourceNameWidth, timeline_txt);
        timeline_txt << std::endl;

        PrintFormattedResourceTimeline("---------", timestamp_width, timeline_txt);
        PrintFormattedResourceTimeline("----------", kEventTypeWidth, timeline_txt);
        PrintFormattedResourceTimeline("-------------", kResourceTypeWidth, timeline_txt);
        PrintFormattedResourceTimeline("-------------------", kResourceIdWidth, timeline_txt);
        PrintFormattedResourceTimeline("-------------", kResourceSizeWidth, timeline_txt);
        PrintFormattedResourceTimeline("-------------", kResourceNameWidth, timeline_txt);
        timeline_txt << std::endl;

        for (size_t index = 0; index < va_info.rgd_resource_timeline_.size(); index++)
        {
            const RgdResourceTimeline& rgd_resource_event = va_info.rgd_resource_timeline_[index];
            const RgdResource& rgd_resource = *va_info.resource_map_[rgd_resource_event.identifier];

            // Upon creation of a committed resource an implicit heap is created or upon a creation of a heap an implicit buffer is created.
            // This pair of committed resource and associated implicit resource will be consolidated and treated as one resource for default RGD output.
            // Skip consolidating implicit resources if '--expand-implicit-resources' option is not used.
            // associated_resource_idx holds the index of the associated RmtResource and implicit resource pair.
            // If '--expand-implicit-resource' option is used, associated_resource_idx will not be updated.
            bool is_implicit_resource = (rgd_resource.associated_resource_idx != -1 && rgd_resource.resource_type == kRmtResourceTypeHeap);

            if (!is_implicit_resource)
            {
                if (index == 0 || look_ahead_counter[index - 1] == 0 || same_events_count == 0)
                {
                    // Set same events count.
                    same_events_count = look_ahead_counter[index];
                    timestamp_txt << GetTimestampString(rgd_resource_event.event_timestamp);
                }

                // Does this resource has associated implicit resource or is the '--expand-implicit-resources' option used?
                bool is_resource_pair_print = (rgd_resource.associated_resource_idx != -1);

                if (is_resource_pair_print)
                {
                    assert(va_info.resource_list_[rgd_resource.associated_resource_idx] != nullptr);
                    if (va_info.resource_list_[rgd_resource.associated_resource_idx] == nullptr)
                    {
                        is_resource_pair_print = false;
                    }
                }

                if (look_ahead_counter[index] == 0 || same_events_count < kMinEventsToExpand)
                {
                    if (same_events_count >= kMinEventsToExpand)
                    {
                        timestamp_txt << ".." << GetTimestampString(rgd_resource_event.event_timestamp) << " (" << same_events_count + 1 << " occurrences)";
                    }

                    PrintFormattedResourceTimeline(timestamp_txt.str(), timestamp_width, timeline_txt);

                    // Clear the timestamp txt stream.
                    timestamp_txt.str(std::string());

                    switch (rgd_resource_event.event_type)
                    {
                    case RgdResourceEventType::kCreate:
                        PrintFormattedResourceTimeline(kResourceEventTypeCreateStr, kEventTypeWidth, timeline_txt);
                        break;

                    case RgdResourceEventType::kBind:
                        PrintFormattedResourceTimeline(kResourceEventTypeBindStr, kEventTypeWidth, timeline_txt);
                        break;

                    case RgdResourceEventType::kMakeResident:
                        PrintFormattedResourceTimeline(kResourceEventTypeMakeResidentStr, kEventTypeWidth, timeline_txt);
                        break;

                    case RgdResourceEventType::kEvict:
                        PrintFormattedResourceTimeline(kResourceEventTypeEvictStr, kEventTypeWidth, timeline_txt);
                        break;

                    case RgdResourceEventType::kDestroy:
                        PrintFormattedResourceTimeline(kResourceEventTypeDestroy, kEventTypeWidth, timeline_txt);
                        break;

                    default:
                        // Should not reach here.
                        assert(false);
                    }

                    std::stringstream resource_type_string;
                    std::stringstream resource_name_string;
                    std::stringstream resource_id_string;
                    if (is_resource_pair_print)
                    {
                        // Print the legend information for '<>' notation.
                        is_print_legend = true;

                        const RgdResource& associated_resource = *va_info.resource_list_[rgd_resource.associated_resource_idx];

                        // This resource has associated implicit resource. Resource type is printed in pair, Example: <Heap, Image>
                        resource_id_string << std::hex << "<0x" << rgd_resource.rmv_identifier << ", 0x" << associated_resource.rmv_identifier << ">" << std::dec;
                        resource_type_string << "<" << GetResourceTypeText(kRmtResourceTypeHeap) << ", " << GetResourceTypeText(rgd_resource.resource_type) << ">";
                        resource_name_string << ((rgd_resource.resource_name != kNullStr) ? rgd_resource.resource_name : associated_resource.resource_name);
                    }
                    else
                    {
                        resource_id_string << std::hex << "0x" << rgd_resource.rmv_identifier << std::dec;
                        resource_type_string << GetResourceTypeText(rgd_resource.resource_type);
                        resource_name_string << rgd_resource.resource_name;
                    }
                    PrintFormattedResourceTimeline(resource_type_string.str(), kResourceTypeWidth, timeline_txt);
                    PrintFormattedResourceTimeline(resource_id_string.str(), kResourceIdWidth, timeline_txt);
                    PrintFormattedResourceTimeline(RgdParsingUtils::GetFormattedSizeString(rgd_resource.size_in_bytes), kResourceSizeWidth, timeline_txt);
                    PrintFormattedResourceTimeline(resource_name_string.str(), kResourceNameWidth, timeline_txt);

                    // Reset same events counts.
                    same_events_count = 0;
                    timeline_txt << std::endl;
                }
            }
        }

        if (is_print_legend)
        {
            txt << legend_txt.str();
        }

        if (cpu_frequency_ != 0)
        {
            txt << timestamp_format_txt.str();
        }
        txt << std::endl;
        txt << timeline_txt.str();
    }
    else
    {
        txt << "INFO: no resource timeline information available." << std::endl;
    }
    txt << std::endl;
    resource_timeline = txt.str();
}

std::string RgdResourceInfoSerializer::pImplResourceInfoSerializer::GetTimestampString(const uint64_t timestamp)
{
    std::stringstream time_txt;

    if (is_raw_time_)
    {
        time_txt << timestamp;
    }
    else
    {
        time_txt << GetRealTime(timestamp);
    }

    return time_txt.str();
}

inline std::string RgdResourceInfoSerializer::pImplResourceInfoSerializer::GetRealTime(uint64_t ticks)
{
    std::stringstream time_txt;
    assert(cpu_frequency_ != 0);
    if (cpu_frequency_ != 0)
    {
        try {
            const uint64_t kTimeBase = 60;
            const uint64_t remainder_timestamp_len = std::to_string(cpu_frequency_ - 1).length();
            uint64_t time_seconds = ticks / cpu_frequency_;
            ticks %= cpu_frequency_;
            uint64_t time_minutes = time_seconds / kTimeBase;
            time_seconds %= kTimeBase;
            uint64_t time_hours = time_minutes / kTimeBase;
            time_minutes %= kTimeBase;
            time_txt << std::right << std::setw(2) << std::setfill('0') << time_hours;
            time_txt << ":" << std::right << std::setw(2) << std::setfill('0') << time_minutes;
            time_txt << ":" << std::right << std::setw(2) << std::setfill('0') << time_seconds;
            time_txt << "." << std::right << std::setw(remainder_timestamp_len) << std::setfill('0') << ticks;
        }
        catch (...)
        {
            is_raw_time_ = true;
            RgdUtils::PrintMessage(kPrintRawTimestampMsg, RgdMessageType::kWarning, true);
            time_txt << ticks;
        }
    }
    else
    {
        is_raw_time_ = true;
        RgdUtils::PrintMessage(kPrintRawTimestampMsg, RgdMessageType::kWarning, true);
        time_txt << ticks;
    }

    return time_txt.str();
}