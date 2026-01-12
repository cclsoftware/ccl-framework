include_guard (DIRECTORY)

find_package (SamsungIAPSDK)

ccl_list_append_once (cclextras_stores_platform_sources
	${CCL_DIR}/extras/stores/platform/android/amazon/amazonstoremanager.android.cpp
	${CCL_DIR}/extras/stores/platform/android/amazon/amazonstoremanager.android.h
	${CCL_DIR}/extras/stores/platform/android/playstore/playstoremanager.android.cpp
	${CCL_DIR}/extras/stores/platform/android/playstore/playstoremanager.android.h
	${CCL_DIR}/extras/stores/platform/android/storemanager.android.cpp
)

ccl_list_append_once (cclextras_stores_java_sources
	${CCL_DIR}/extras/stores/platform/android/amazon/AmazonStoreContext.java
	${CCL_DIR}/extras/stores/platform/android/playstore/PlayStoreContext.java
)

if (SamsungIAPSDK_FOUND)
	ccl_list_append_once (cclextras_stores_platform_sources
		${CCL_DIR}/extras/stores/platform/android/samsung/samsungstoremanager.android.cpp
		${CCL_DIR}/extras/stores/platform/android/samsung/samsungstoremanager.android.h
	)

	ccl_list_append_once (cclextras_stores_java_sources
		${CCL_DIR}/extras/stores/platform/android/samsung/SamsungStoreContext.java
	)
endif ()

source_group ("source/platform" FILES ${cclextras_stores_platform_sources})
source_group ("source/platform/java" FILES ${cclextras_stores_java_sources})

ccl_list_append_once (cclextras_stores_sources
	${cclextras_stores_platform_sources}
	${cclextras_stores_java_sources}
)

ccl_add_aar_project (cclextras-stores NAMESPACE "dev.ccl.cclextras.stores" DEPENDS cclgui)
ccl_install_aar (cclextras-stores COMPONENT prebuilt_libraries_${VENDOR_NATIVE_COMPONENT_SUFFIX})

ccl_add_gradle_dependency (cclextras-stores "com.android.billingclient:billing:8.0.0" TRANSITIVE)
ccl_add_gradle_dependency (cclextras-stores "com.amazon.device:amazon-appstore-sdk:3.0.7" TRANSITIVE)

ccl_add_java_sourcedir (cclextras-stores "${CCL_DIR}/extras/stores/platform/android/playstore")
ccl_add_java_sourcedir (cclextras-stores "${CCL_DIR}/extras/stores/platform/android/amazon")

if (SamsungIAPSDK_FOUND)
	target_compile_definitions (cclextras-stores PRIVATE "CCL_SAMSUNG_STORE_MANAGER_ENABLED=1")

	ccl_add_gradle_dependency (cclextras-stores "${SamsungIAPSDK_AAR}" COMPILE_ONLY)
	ccl_add_gradle_dependency (cclextras-stores "org.jetbrains:annotations:26.0.2" TRANSITIVE)

	ccl_add_java_sourcedir (cclextras-stores "${CCL_DIR}/extras/stores/platform/android/samsung")
endif ()

ccl_add_proguard_file (cclextras-stores "${CCL_DIR}/packaging/android/cclstore.proguard")
