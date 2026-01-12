#define MAIN_MACRO(a) \
{}

/* Verify condition from file1.json config */

#ifdef FILE1_CONDITION
	#define MAIN_FILE1_CONDITION_TRUE_MACRO(a,b,c) {}
#else
	#define MAIN_FILE1_CONDITION_FALSE_MACRO(a,b,c) {}
#endif

/* Verify condition from file2.json config */

#ifdef FILE2_CONDITION
	#define MAIN_FILE2_CONDITION_TRUE_MACRO(a,b,c) {}
#else
	#define MAIN_FILE2_CONDITION_FALSE_MACRO(a,b,c) {}
#endif