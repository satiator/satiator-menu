int boot_disc(void);

// return codes for boot_disc
#define BOOT_BAD_HEADER         -1
#define BOOT_BAD_SECURITY_CODE  -4
#define BOOT_BAD_REGION         -8

#define BOOT_UNRECOGNISED_BIOS  -1024

const char *get_region_string(void);    // must only call in init.c before per_init()
extern const char *region_string;       // set in init.c, read from anywhere
