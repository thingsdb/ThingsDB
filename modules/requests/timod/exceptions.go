package timod

//Ex is used as protocol type used by ThingsDB.
type Ex int8

const (
	//ExOperationError operation is not valid in the current context
	ExOperationError Ex = -63

	//ExNumArguments wrong number of arguments
	ExNumArguments Ex = -62

	//ExTypeError object of inappropriate type
	ExTypeError Ex = -61

	//ExValueError object has the right type but an inappropriate value
	ExValueError Ex = -60

	//ExOverflow integer overflow
	ExOverflow Ex = -59

	//ExZeroDiv division or module by zero
	ExZeroDiv Ex = -58

	//ExMaxQuota max quota is reached
	ExMaxQuota Ex = -57

	//ExAuthError authentication error
	ExAuthError Ex = -56

	//ExForbidden forbidden (access denied)
	ExForbidden Ex = -55

	//ExLookupError requested resource not found
	ExLookupError Ex = -54

	//ExBadData unable to handle request due to invalid data
	ExBadData Ex = -53

	//ExSyntaxError syntax error in query
	ExSyntaxError = -52

	//ExNodeError node is temporary unable to handle the request
	ExNodeError = -51

	//ExAssertError assertion statement has failed
	ExAssertError = -50

	//ExCustom127 can be used as a custom error
	ExCustom127 Ex = -127

	//ExCustom126 can be used as a custom error
	ExCustom126 Ex = -126

	//ExCustom125 can be used as a custom error
	ExCustom125 Ex = -125

	//ExCustom124 can be used as a custom error
	ExCustom124 Ex = -124

	//ExCustom123 can be used as a custom error
	ExCustom123 Ex = -123

	//ExCustom122 can be used as a custom error
	ExCustom122 Ex = -122

	//ExCustom121 can be used as a custom error
	ExCustom121 Ex = -121

	//ExCustom120 can be used as a custom error
	ExCustom120 Ex = -120

	//ExCustom119 can be used as a custom error
	ExCustom119 Ex = -119

	//ExCustom118 can be used as a custom error
	ExCustom118 Ex = -118

	//ExCustom117 can be used as a custom error
	ExCustom117 Ex = -117

	//ExCustom116 can be used as a custom error
	ExCustom116 Ex = -116

	//ExCustom115 can be used as a custom error
	ExCustom115 Ex = -115

	//ExCustom114 can be used as a custom error
	ExCustom114 Ex = -114

	//ExCustom113 can be used as a custom error
	ExCustom113 Ex = -113

	//ExCustom112 can be used as a custom error
	ExCustom112 Ex = -112

	//ExCustom111 can be used as a custom error
	ExCustom111 Ex = -111

	//ExCustom110 can be used as a custom error
	ExCustom110 Ex = -110

	//ExCustom109 can be used as a custom error
	ExCustom109 Ex = -109

	//ExCustom108 can be used as a custom error
	ExCustom108 Ex = -108

	//ExCustom107 can be used as a custom error
	ExCustom107 Ex = -107

	//ExCustom106 can be used as a custom error
	ExCustom106 Ex = -106

	//ExCustom105 can be used as a custom error
	ExCustom105 Ex = -105

	//ExCustom104 can be used as a custom error
	ExCustom104 Ex = -104

	//ExCustom103 can be used as a custom error
	ExCustom103 Ex = -103

	//ExCustom102 can be used as a custom error
	ExCustom102 Ex = -102

	//ExCustom101 can be used as a custom error
	ExCustom101 Ex = -101

	//ExCustom100 can be used as a custom error
	ExCustom100 Ex = -100
)
