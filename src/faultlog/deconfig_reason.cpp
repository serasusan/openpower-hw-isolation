#include <deconfig_reason.hpp>

namespace openpower::faultlog
{
std::string getDeconfigReason(const DeconfiguredByReason& reason)
{
    switch (reason)
    {
        case DECONFIGURED_BY_MANUAL_GARD:
            return "MANUAL";
        case DECONFIGURED_BY_FIELD_CORE_OVERRIDE:
            return "FIELD CORE OVERRIDE";
        case DECONFIGURED_BY_MEMORY_CONFIG:
            return "MEMORY CONFIG";
        case DECONFIGURED_BY_NO_CHILD_MCA:
            return "NO CHILD DIMM";
        case DECONFIGURED_BY_BUS_DECONFIG:
            return "BUS DECONFIGURED";
        case DECONFIGURED_BY_PRD:
            return "PRD";
        case DECONFIGURED_BY_PHYP:
            return "PHYP";
        case DECONFIGURED_BY_SPCN:
            return "SPCN";
        case CONFIGURED_BY_RESOURCE_RECOVERY:
            return "RESOURCE RECOVERY";
        case DECONFIGURED_BY_NO_PARENT_MEMBUF:
        case DECONFIGURED_BY_NO_CHILD_DIMM:
        case DECONFIGURED_BY_NO_PARENT_DMI:
        case DECONFIGURED_BY_NO_PARENT_MBA:
        case DECONFIGURED_BY_EQ_DECONFIG:
        case DECONFIGURED_BY_FC_DECONFIG:
        case DECONFIGURED_BY_CORE_DECONFIG:
        case DECONFIGURED_BY_PHB_DECONFIG:
        case DECONFIGURED_BY_PEC_DECONFIG:
        case DECONFIGURED_BY_NO_CHILD_MCS:
        case DECONFIGURED_BY_NO_PARENT_MCBIST:
        case DECONFIGURED_BY_DISABLED_PORT:
        case DECONFIGURED_BY_NO_CHILD_MI:
        case DECONFIGURED_BY_NO_CHILD_DMI:
        case DECONFIGURED_BY_NO_PARENT_MC:
        case DECONFIGURED_BY_NO_PARENT_MI:
        case DECONFIGURED_BY_NO_MATCHING_LINK_SET:
        case DECONFIGURED_BY_OBUS_MODE:
        case DECONFIGURED_BY_NO_CHILD_OMI:
        case DECONFIGURED_BY_NO_PARENT_MCC:
        case DECONFIGURED_BY_NO_CHILD_MEM_PORT:
        case DECONFIGURED_BY_NO_PARENT_OMI:
        case DECONFIGURED_BY_NO_CHILD_OCMB_CHIP:
        case DECONFIGURED_BY_NO_PARENT_OCMB_CHIP:
        case DECONFIGURED_BY_NO_PARENT_OMIC:
        case DECONFIGURED_BY_INACTIVE_PAU:
        case DECONFIGURED_BY_NO_CHILD_OMIC:
        case DECONFIGURED_BY_NO_CHILD_MCC:
        case DECONFIGURED_BY_NO_PARENT_MEM_PORT:
        case DECONFIGURED_BY_NO_PARENT_PAUC:
        case DECONFIGURED_BY_NO_CHILD_PMIC:
        case DECONFIGURED_BY_NO_PEER_TARGET:
            return "ASSOCIATION";
        default:
            return "ERROR LOG";
    }
    return "ERROR LOG";
}

} // namespace openpower::faultlog
