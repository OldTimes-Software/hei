/**
 * Hei Platform Library
 * Copyright (C) 2017-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is licensed under MIT. See LICENSE for more details.
 */

#include <vulkan/vulkan.h>

#define PRINT_VAR_VERSION( VERSION ) GfxLog( PL_TOSTRING( VERSION ) ": %d.%d.%d\n", VK_VERSION_MAJOR( VERSION ), VK_VERSION_MINOR( VERSION ), VK_VERSION_PATCH( VERSION ) )
#define PRINT_VAR_INTEGER( INTEGER ) GfxLog( PL_TOSTRING( INTEGER ) ": %d\n", INTEGER )

static VkInstance vk_instance = NULL;
static VkPhysicalDevice vk_physicalDevice = VK_NULL_HANDLE;
static VkDevice vk_device = NULL;

static VkExtensionProperties *vk_extensionProperties = NULL;
static unsigned int vk_numExtensions = 0;

static const char *vk_layers[] = {
#if !defined( NDEBUG )
		/* push validation layer if we're debugging */
		"VK_LAYER_KHRONOS_validation"
#endif
};

typedef struct QueueFamilyIndex {
	uint32_t graphics, present;
} QueueFamilyIndex;

static VkInstance VK_CreateInstance( void ) {
	static const VkApplicationInfo appInfo = {
			.apiVersion = VK_API_VERSION_1_2,
			.applicationVersion = VK_MAKE_VERSION( PL_VERSION_MAJOR, PL_VERSION_MINOR, PL_VERSION_PATCH ),
			.engineVersion = VK_MAKE_VERSION( PL_VERSION_MAJOR, PL_VERSION_MINOR, PL_VERSION_PATCH ),
			.pApplicationName = "libplatform",
			.pEngineName = "libplatform",
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO, /* why? ... */
			.pNext = NULL,
	};
	static const VkInstanceCreateInfo createInfo = {
			.pNext = NULL,
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.enabledExtensionCount = 0,
			.flags = 0,
			.pApplicationInfo = &appInfo,
			.ppEnabledExtensionNames = NULL,
			/* push validation layer if we're debugging */
			.ppEnabledLayerNames = vk_layers,
			.enabledLayerCount = plArrayElements( vk_layers ),
	};

	GfxLog( "Creating Vulkan instance...\n" );

	VkInstance instance;
	if( vkCreateInstance( &createInfo, NULL, &instance ) != VK_SUCCESS ) {
		ReportError( PL_RESULT_GRAPHICSINIT, "failed to create Vulkan instance" );
		return NULL;
	}

	GfxLog( "Vulkan instance created!\n" );

	return instance;
}

/**
 * Iterate through and find the best possible device.
 */
static VkPhysicalDevice VK_SelectPhysicalDevice( void ) {
	GfxLog( "Fetching physical devices...\n" );

	unsigned int numDevices;
	vkEnumeratePhysicalDevices( vk_instance, &numDevices, NULL );
	if ( numDevices == 0 ) {
		ReportError( PL_RESULT_GRAPHICSINIT, "no supported physical devices available" );
		return NULL;
	}

	GfxLog( "Found %d graphics devices, selecting appropriate device...\n", numDevices );

	VkPhysicalDevice *physicalDevices = PlMAllocA( sizeof( VkPhysicalDevice ) * numDevices );
	vkEnumeratePhysicalDevices( vk_instance, &numDevices, physicalDevices );

	VkPhysicalDeviceProperties properties;
	for ( unsigned int i = 0; i < numDevices; ++i ) {
		if ( i == 0 ) {
			vk_physicalDevice = physicalDevices[ i ];
			vkGetPhysicalDeviceProperties( vk_physicalDevice, &properties );
			continue;
		}

		/* compare this device and the device we'd like to use */
		VkPhysicalDeviceProperties newProps;
		vkGetPhysicalDeviceProperties( physicalDevices[ i ], &newProps );
		if ( newProps.limits.maxImageDimension2D > properties.limits.maxImageDimension2D ) {
			vk_physicalDevice = physicalDevices[ i ];
			properties = newProps;
		}
	}

	GfxLog( "Selected device, \"%s\"\n", properties.deviceName );
	PRINT_VAR_VERSION( properties.apiVersion );
	PRINT_VAR_VERSION( properties.driverVersion );
	PRINT_VAR_INTEGER( properties.deviceID );
	PRINT_VAR_INTEGER( properties.vendorID );

	PlFree( physicalDevices );

	return vk_physicalDevice;
}

static VkDevice VK_CreateLogicalDevice( void ) {
	VkDeviceQueueCreateInfo queueCreateInfo;
	memset( &queueCreateInfo, 0, sizeof( VkDeviceQueueCreateInfo ) );
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

	float priority = 1.0f;
	queueCreateInfo.pQueuePriorities = &priority;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.queueFamilyIndex = -1;

	VkDeviceCreateInfo createInfo;
	memset( &createInfo, 0, sizeof( VkDeviceCreateInfo ) );
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	VkPhysicalDeviceFeatures deviceFeatures;

	if ( vkCreateDevice( vk_physicalDevice, &createInfo, NULL, &vk_device ) != VK_SUCCESS ) {
		ReportError( PL_RESULT_GRAPHICSINIT, "failed to create device" );
		return NULL;
	}
}

void plInitVulkan( void ) {
	FunctionStart();

	/* attempt to create the vulkan instance */
	vk_instance = VK_CreateInstance();
	if ( vk_instance == NULL ) {
		return;
	}

	/* fetch a physical device */
	vk_physicalDevice = VK_SelectPhysicalDevice();
	if ( vk_physicalDevice == NULL ) {
		return;
	}

	/* query vulkan extensions */
	vkEnumerateInstanceExtensionProperties( NULL, &vk_numExtensions, NULL );
	vk_extensionProperties = PlMAllocA( sizeof( VkExtensionProperties ) * vk_numExtensions );
	vkEnumerateInstanceExtensionProperties( NULL, &vk_numExtensions, vk_extensionProperties );
	GfxLog( "  extensions:\n" );
	for( unsigned int i = 0; i < vk_numExtensions; ++i ) {
		GfxLog( "    %s\n", vk_extensionProperties[ i ].extensionName );
	}
}

void plShutdownVulkan( void ) {
	PlFree( vk_extensionProperties );
	vk_numExtensions = 0;

	if ( vk_instance != NULL ) {
		vkDestroyInstance( vk_instance, NULL );
		vk_instance = NULL;
	}

	if ( vk_device != NULL ) {
		vkDestroyDevice( vk_device, NULL );
		vk_device = NULL;
	}
}
