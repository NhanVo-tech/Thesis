.l1_config_persist_storage_sha256 CONFIG_SECURE_PARTITIONS_UWB_L1_CONFIG_SHA256_BASE_ADDRESS (NOLOAD) :
{
	__l1_config_persist_storage_sha256_start = .;
	KEEP(*(.l1_config_persist_storage_sha256))
	. =  __l1_config_persist_storage_sha256_start + CONFIG_SECURE_PARTITIONS_UWB_L1_CONFIG_SHA256_SIZE;
	__l1_config_persist_storage_sha256 = .;
} > SECSTORAGE

.l1_config_persist_storage CONFIG_SECURE_PARTITIONS_UWB_L1_CONFIG_BASE_ADDRESS (NOLOAD) :
{
	__l1_config_persist_storage_start = .;
	KEEP(*(.l1_config_persist_storage))
	. =  __l1_config_persist_storage_start + CONFIG_SECURE_PARTITIONS_UWB_L1_CONFIG_SIZE;
	__l1_config_persist_storage_end = .;
} > SECSTORAGE
