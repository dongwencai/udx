#ifndef __UDX_XML_H__
#define __UDX_XML_H__

typedef struct udx_msg UDX_MSG_S;

int udx_process_response(int * pRet);
int udx_generate_request(UDX_MSG_S * pMsg, char ** ret_buf, int *ret_len);
int udx_process_xml_msg(const char * xml_buf, int total_len, char ** ret_buf, int *ret_len);

#endif
