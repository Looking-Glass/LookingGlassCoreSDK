/*
    HoloPlayCore 0.1.0

    Intended for use with HoloPlay Service 1.0.1 and above.

    File: include/HoloPlayCore.h

    Author: Evan Kahn
    Contact: evan@lookingglassfactory.com

     This header contains declarations for functions in the HoloPlayCore dynamic library,
    which handles communication with the HoloPlay Service runtime.
    
     A programmer looking to design a custom renderer for Looking Glass devices may not need to
    understand the functions and types declared in this header file. They handle communication
    with HoloPlay Service via the NNG interprocess communication library and provide abstractions
    for the CBOR-serialized messages returned from it, to provide easy access from both low- and
    high- level languages.

     Convenience functions that implement these under the hood are declared in HoloPlayCore.h.
*/

#ifdef __cplusplus
extern "C"
{
#endif
    /*
       Note on string return functions:

        Any function of the form "size_t hpc_DoThing(..., char* out_buf, size_t out_buf_sz)" copies a
       string into a preallocated buffer of size out_buf_sz. The buffer is allocated and owned by
       the caller.

        These functions return 0 if out_buf_sz was sufficient, or the actual size of string if
       out_buf_sz was too small.

        This makes it much easier to pass strings into managed memory (i.e. for .NET bindings).
       Most of the strings returned by these functions are quite small.
    */

    /*
       hpc_client_error

        Enum definition for errors returned from the HoloPlayCore dynamic library.
       
        This encapsulates potential errors with the connection itself,
       as opposed to hpc_service_error, which describes potential error messages
       included in a successful reply from HoloPlay Service.
    */
    typedef enum _hpc_client_error
    {
        hpc_CLIERR_NOERROR,
        hpc_CLIERR_NOSERVICE,
        hpc_CLIERR_VERSIONERR,
        hpc_CLIERR_SERIALIZEERR,
        hpc_CLIERR_DESERIALIZEERR,
        hpc_CLIERR_MSGTOOBIG,
        hpc_CLIERR_SENDTIMEOUT,
        hpc_CLIERR_RECVTIMEOUT,
        hpc_CLIERR_PIPEERROR,
        hpc_CLIERR_APPNOTINITIALIZED
    } hpc_client_error;

    /*
       hpc_service_error

        Enum definition for error codes included in HoloPlay Service responses.

        Most error messages from HoloPlay Service concern access to the HoloPlay Service
       internal renderer, which is supported but not the primary focus of the current
       version of HoloPlay Core.
        
        Future versions of HoloPlay Service may return error codes not defined by this
       spec.
    */
    typedef enum _hpc_service_error
    {
        hpc_ERR_NOERROR,
        hpc_ERR_BADCBOR,
        hpc_ERR_BADCOMMAND,
        hpc_ERR_NOIMAGE,
        hpc_ERR_LKGNOTFOUND,
        hpc_ERR_NOTINCACHE,
        hpc_ERR_INITTOOLATE,
        hpc_ERR_NOTALLOWED,
    } hpc_service_error;

    /*
       hpc_license_type

        Enum definition for possible types of licenses associated with a HoloPlay Core app.
        
        Non-commercial apps can't run on Looking Glass devices without an associated commercial license.
    */
    typedef enum _hpc_license_type
    {
        hpc_LICENSE_NONCOMMERCIAL,
        hpc_LICENSE_COMMERCIAL
    } hpc_license_type;

    /*
       hpc_obj

        Opaque pointer type that references an object from the jsoncons::json C++ library
       (https://github.com/danielaparker/jsoncons/), a static dependency of HoloPlay Core.
       Objects of this type can be sent to and received from HoloPlay Service, and manipulated
       without an external deserializer using the other functions declared in this file.
    */
    typedef void *hpc_obj;

    /*
       hpc_StateMsgInstance
       
        Args: none
        Returns: pointer to the global state variable from HoloPlay Service. Null if no state
       message has been requested. For more information on the global state message, see
       hpc_InitializeApp and hpc_RefreshState in HoloPlayCore.h.
    */
    IMPORT_DECL hpc_obj *hpc_StateMsgInstance();

    /*
       hpc_MakeObject

        Serializes, allocates, and constructs an opaque hpc_obj, which can be sent to HoloPlay Service. 
       Messages must include a command part, which will be converted from JSON, and an optional
       binary field, which can include quilt data to send to the HoloPlay Service internal renderer.

        Args: JSON command string; binary message length; binary start pointer.
        Returns: opaque pointer to the created object. Null if cmd is not a valid JSON string.
    */
    IMPORT_DECL hpc_obj *hpc_MakeObject(const char *cmd_json, const size_t binlen, const unsigned char *bin);

    /*
       hpc_DeleteObject

        Calls the C++ destructor of an hpc_object. 

        Args: object to destroy
        Returns: none
    */
    IMPORT_DECL void hpc_DeleteObject(hpc_obj *obj);

    /*
       hpc_ObjQuery functions

        This group of functions returns primitive values queried from opaque hpc_obj items.

        They are just C wrappers for deserialization calls in the C++ jsoncons dependency,
       using the jsonpointer query syntax to request values from inside a hierarchy.
       More information on jsonpointer can be found here: https://tools.ietf.org/html/rfc6901

        Args: object to query; jsonpointer query string
        Returns: query result; 0 if query has failed.
    */
    
    IMPORT_DECL int hpc_ObjQueryInt(const hpc_obj *obj, const char *query_string);
    IMPORT_DECL float hpc_ObjQueryFloat(const hpc_obj *obj, const char *query_string);

    /*
       hpc_ObjQueryString

        Args: object to query; jsonpointer query string; return buffer and size (see note on
       string return functions). out_buf will be unmodified if query fails.
    */
    IMPORT_DECL size_t hpc_ObjQueryString(const hpc_obj *obj, const char *query_string, char *out_buf, size_t out_buf_sz);

    /*
       hpc_ObjGetLength

        Args: object to query; jsonpointer query string
        Returns: length of JSON subvalue - ONLY if it is an array type. 0 otherwise.
    */
    IMPORT_DECL int hpc_ObjGetLength(const hpc_obj *obj, const char *query_string);

    /*
       hpc_ObjAsJson

        Args: object to query; jsonpointer query string
        Returns: JSON serialized string representation of object (see note on string return
       functions).
    */
    IMPORT_DECL size_t hpc_ObjAsJson(const hpc_obj *obj, char *out_buf, size_t out_buf_sz);

    /*
       hpc_ObjGetErrorCode

        Args: object to query
        Returns: HoloPlay Service error message associated with object. (Obviously, this
       only makes sense when applied to an hpc_obj returned from a HoloPlay Service response).
    */
    IMPORT_DECL hpc_service_error hpc_ObjGetErrorCode(const hpc_obj *message);
    
    /*
       hpc_GetDeviceProperty functions

        This group of functions returns primitive values queried from a specific device
       described in the global state object.

        Args: index of device to query
        Returns: query result (0 if query has failed). See note on string return functions
       for hpc_GetDevicePropertyString.
    */
    IMPORT_DECL int hpc_GetDevicePropertyInt(int dev_index, const char *query_string);
    IMPORT_DECL float hpc_GetDevicePropertyFloat(int dev_index, const char *query_string);
    IMPORT_DECL size_t hpc_GetDevicePropertyString(int dev_index, const char *query_string, char *out_buf, size_t out_buf_sz);

    /*
       hpc_Send functions

        This group of functions sends messages to HoloPlay Service and allocates and 
       retrieves the response.
    */

    /*
       hpc_SendCallback

        This function sends an asynchronous message to HoloPlay Service and returns an allocated
       response as an argument to the provided callback function.

        Callback functions should be of the form:
       void cb(hpc_obj obj, hpc_client_error clierr, void *context)

        Args: request object; callback function name; context pointer
        Returns: client error (only if the error occurs with sending the message; errors
       incurred in receiving the response will be passed as a parameter into the callback
       function).
    */
    IMPORT_DECL hpc_client_error hpc_SendCallback(const hpc_obj *request, void (*callback)(hpc_obj, hpc_client_error, void *), void *context);

    /*
       hpc_SendBlocking

        This function sends a synchronous message to HoloPlay Service and returns an allocated
       response.

        Args: request object; pointer to hpc_obj in which to store the response. The caller
       need not allocate memory for the response object.
        Returns: client error
    */
    IMPORT_DECL hpc_client_error hpc_SendBlocking(const hpc_obj *request, hpc_obj **response);

    /*
       hpc_SetupMessagePipe

        Function to set up the connection to HoloPlay Service, without sending any messages.
       This will automatically be called when the first message is sent, but it can also be called
       earlier.
        There are very few reasons to call this directly.

        Args: none
        Returns: 0 if successful or 1 if the connection already exists.
    */
    IMPORT_DECL int hpc_SetupMessagePipe();

    /*
       hpc_TeardownMessagePipe

        Function to sever the connection to HoloPlay Service.
       This is called by hpc_CloseApp, so there is little reason to call it directly.

        Args: none
        Returns: 0 if successful or 1 if the connection is already closed.
    */
    IMPORT_DECL int hpc_TeardownMessagePipe();

#ifdef __cplusplus
}
#endif
