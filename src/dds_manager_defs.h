#ifndef __DDS_MANAGER_DEFS__
#define __DDS_MANAGER_DEFS__

// Define DLL_PUBLIC as the import or export symbol declaration
#if defined DDSMAN_STATIC_BUILD
  #define DLL_PUBLIC
#elif defined WIN32 || defined WIN64
  #ifdef OpenDDSManager_EXPORTS // Set automatically by CMake
    #define DLL_PUBLIC __declspec(dllexport)
  #else
    #define DLL_PUBLIC __declspec(dllimport)
  #endif
#else
  #define DLL_PUBLIC __attribute__ ((visibility("default")))
#endif

#endif

/**
 * @}
 */
