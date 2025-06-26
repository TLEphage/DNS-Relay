#include "dnsStruct.h"
/*DNS协议部分*/

//解析DNS报文
void parse_dns_packet(DNS_message *msg,const char *buffer,int length){
    if(length<12){
        printf("DNS报文太短\n");
        return;
    }
    //Parse DNS header
    unsigned short transactionID=(buffer[0]<<8)|buffer[1];
    unsigned short flags=(buffer[2]<<8)|buffer[3];
    unsigned short qdCount=(buffer[4]<<8)|buffer[5];
    unsigned short anCount=(buffer[6]<<8)|buffer[7];
    unsigned short nsCount=(buffer[8]<<8)|buffer[9];
    unsigned short arCount=(buffer[10]<<8)|buffer[11];

    msg->header=malloc(sizeof(DNS_header));
    if(!msg->header)
    {
        printf("DNS报文头部内存分配失败\n");
        return;
    }
    msg->header->transactionID=transactionID;
    msg->header->flags=flags;
    msg->header->ques_num=qdCount;
    msg->header->ans_num=anCount;
    msg->header->auth_num=nsCount;
    msg->header->add_num=arCount;

    int offset=12;
    //Parse questions
    if(qdCount>0)
    {
        msg->question=malloc(qdCount * sizeof(DNS_question));
        if(!msg->question)
        {
            printf("DNS报文问题部分内存分配失败\n");
            return;
        }
        for(int i=0;i<qdCount;i++){
            printf("\nQuestion #%d:\n",i+1);
            msg->question[i].qname=parse_dns_name(buffer,&offset,length);
            if(!msg->question[i].qname)
            {
                printf("DNS报文问题部分解析失败\n");
                return;
            }
            printf(" Name:%s\n",msg->question[i].qname);

            if(offset+4>length){
                printf("DNS报文问题部分不完整.\n");
                return;
            }

            msg->question[i].qtype=(buffer[offset]<<8)|buffer[offset+1];
            msg->question[i].qclass=(buffer[offset+2]<<8)|buffer[offset+3];
            offset+=4;

            printf(" Type:%u\n",msg->question[i].qtype);
            printf(" Class:%u\n",msg->question[i].qclass);
        }
    }

            //Parse answers
    if (anCount > 0)
    {
        msg->answer = malloc(anCount * sizeof(DNS_resource_record));
        if (!msg->answer)
        {
            printf("Memory allocation failed for answers.\n");
            return;
        }

        printf("\nAnswers:\n");
        for (int i = 0; i < anCount; i++)
        {
            printf("\nAnswer #%d:\n", i + 1);
            parse_resource_record(buffer, &offset, length, &msg->answer[i]);
        }
    }
        // Parse authority section
    if(nsCount>0)
    {
        msg->authority=malloc(nsCount * sizeof(DNS_resource_record));
        if(!msg->authority)
        {
            printf("Memory allocation failed for authority records.\n");
            return;
        }

        printf("\nAuthority records:\n");
        for(int i=0;i<nsCount;i++)
        {
            printf("\nAuthority Records #%d:\n", i + 1);
            parse_resource_record(buffer,&offset,length,&msg->authority[i]);

        }
    }
        //Parse additional records
        if(arCount>0)
        {
            msg->add=malloc(arCount * sizeof(DNS_resource_record));
            if(!msg->add)
            {
                printf("Memory allocation failed for additional records.\n");
                return;
            }

            printf("\nAddtional records:\n");
            for(int i=0;i<arCount;i++){
                printf("\nAdditional Record #%d:\n",i+1);
                parse_resource_record(buffer,&offset,length,&msg->add[i]);
            }
        }

}

/*关键DNS域名压缩*/
/*创建 256 字节缓冲区存储域名（DNS 域名最大长度限制）
初始化位置指针和偏移量跟踪
设置跳转计数器防止无限循环*/
char *parse_dns_name(const char*buffer,int*offset,int max_length){
    char name[256];
    int name_pos=0;
    int initial_offset=*offset;
    int jumps=0;

    while(1){
        if(*offset>=max_length){
            return NULL;
        }

        unsigned int len=(unsigned char)buffer[(*offset)++];
//检查单比特
        if(len==0){
            if(name_pos==0){
                name[name_pos++]='.';
            }
            name[name_pos]='\0';
            break;
        }

        //检查双字节以上
        if((len&0xc0)==0xc0){
            if(*offset>=max_length){
                return NULL;
            }

            //获取偏移指针
            int ptr_offset=((len&0x3F)<<8)|(unsigned char)buffer[(*offset)++];
            if(ptr_offset>=initial_offset)
            {
                return NULL;
            }

            if(jumps++>10){
                return NULL;
            }

            int saved_offset=*offset;
            *offset=ptr_offset;
            char *rest=parse_dns_name(buffer,offset,max_length);
            *offset=saved_offset;

            if(!rest){
                return NULL;
            }

            if(name_pos>0){
                name[name_pos++]='.';
            }
            strcpy(name+name_pos,rest);
            name_pos+=strlen(rest);
            free(rest);
            break;
        }

        //正常标签
        if(name_pos>0){
            name[name_pos++]='.';
        }

        if(*offset+len>max_length)
        {
            return NULL;
        }

        memcpy(name+name_pos,buffer+*offset,len);
        *offset+=len;
        name_pos+=len;

    }
    return strdup(name);
}


/*解析DNS资源记录的辅助函数*/
void parse_resource_record(const char*buffer,int *offset,int max_length,DNS_resource_record *rr)
{
    rr->name=parse_dns_name(buffer,offset,max_length);
    if(!rr->name){
        printf("Failed to parse resource record name.\n");
        return;
    }
    printf(" Name:%s\n",rr->name);

    if(*offset+10>max_length){
        printf("Incomplete resource record.\n");
        return;
    }

    rr->type=(buffer[*offset]<<8)|buffer[*offset+1];
    rr->class=(buffer[*offset+2]<<8)|buffer[*offset+3];
    rr->ttl=(buffer[*offset+4]<<24)|(buffer[*offset+5]<<16)|(buffer[*offset+6]<<8)|buffer[*offset+7];
    rr->rdlength=(buffer[*offset+8]<<8)|buffer[*offset+9];
    *offset+=10;

    printf(" Type:%u\n",rr->type);
    printf(" Class:%u\n",rr->class);
    printf(" TTL:%u\n",rr->ttl);
    printf(" RD Length:%u\n",rr->rdlength);

    if(*offset+rr->rdlength>max_length)
    {
        printf("Invalid RDATA length.\n");
        return;
    }

    //解析资源数据（Rdata)
    switch(rr->type)
    {
        case 1: //A记录(IPV4)
            if(rr->rdlength!=4){
                for(int i=0;i<4;i++){
                    rr->data.a_record.IP_addr[i]=buffer[(*offset)++];
                }
                printf(" Address: %u.%u.%u.%u\n",rr->data.a_record.IP_addr[0],rr->data.a_record.IP_addr[1],
                rr->data.a_record.IP_addr[2],rr->data.a_record.IP_addr[3]);
            }
            break;

        case 28: //AAAA记录(IPV6)
            if(rr->rdlength==16)
            {
                for(int i=0;i<16;i++){
                    rr->data.aaa_record.IP_addr[i]=buffer[(*offset)++];
                }
                printf("Address:");
                for (int i = 0; i < 16; i++)
            {
                printf("%02x", rr->data.aaa_record.IP_addr[i]);
                if (i % 2 == 1 && i != 15)
                    printf(":");
            }
            printf("\n");
            }
            break;
        
        case 5: //CNAME记录
        rr->data.cname_record.name = parse_dns_name(buffer, offset, max_length);
        if (rr->data.cname_record.name)
        {
            printf("  CNAME: %s\n", rr->data.cname_record.name);
        }
        break;

    case 6: // SOA record
        rr->data.soa_record.MName = parse_dns_name(buffer, offset, max_length);
        rr->data.soa_record.RName = parse_dns_name(buffer, offset, max_length);
        if (*offset + 20 > max_length)
        {
            printf("Incomplete SOA record.\n");
            return;
        }
        rr->data.soa_record.serial = (buffer[*offset] << 24) | (buffer[*offset + 1] << 16) |
                                     (buffer[*offset + 2] << 8) | buffer[*offset + 3];
        rr->data.soa_record.refresh = (buffer[*offset + 4] << 24) | (buffer[*offset + 5] << 16) |
                                      (buffer[*offset + 6] << 8) | buffer[*offset + 7];
        rr->data.soa_record.retry = (buffer[*offset + 8] << 24) | (buffer[*offset + 9] << 16) |
                                    (buffer[*offset + 10] << 8) | buffer[*offset + 11];
        rr->data.soa_record.expire = (buffer[*offset + 12] << 24) | (buffer[*offset + 13] << 16) |
                                     (buffer[*offset + 14] << 8) | buffer[*offset + 15];
        rr->data.soa_record.minimum = (buffer[*offset + 16] << 24) | (buffer[*offset + 17] << 16) |
                                      (buffer[*offset + 18] << 8) | buffer[*offset + 19];
        *offset += 20;

        printf("  SOA MName: %s\n", rr->data.soa_record.MName);
        printf("  SOA RName: %s\n", rr->data.soa_record.RName);
        printf("  SOA Serial: %u\n", rr->data.soa_record.serial);
        printf("  SOA Refresh: %u\n", rr->data.soa_record.refresh);
        printf("  SOA Retry: %u\n", rr->data.soa_record.retry);
        printf("  SOA Expire: %u\n", rr->data.soa_record.expire);
        printf("  SOA Minimum: %u\n", rr->data.soa_record.minimum);
        break;

    default:
        // For unsupported types, just skip the data
        printf("  Unsupported record type, skipping data.\n");
        *offset += rr->rdlength;
        break;
    
    }
}


//构建DNS响应报文
int build_dns_response(unsigned char *request, int requestLen, const char *ip)
{
    // 假设你已经解析出请求中的 ID、问题部分等内容
    // 这里只是简单构造一个响应示意，不完整！

    unsigned char *response = request; // 直接在原缓冲区构造
    // 设置响应标志位
    response[2] |= 0x80; // QR = 1 表示响应
    response[3] = 0x80;  // 递归可用

    response[7] = 0x01; // ANCOUNT = 1，回答数量为1

    int offset = requestLen; // 在原请求基础上添加

    // 构造回答部分（压缩名指针）
    response[offset++] = 0xC0;
    response[offset++] = 0x0C; // Name 指针指向问题部分

    response[offset++] = 0x00;
    response[offset++] = 0x01; // Type A
    response[offset++] = 0x00;
    response[offset++] = 0x01; // Class IN
    response[offset++] = 0x00;
    response[offset++] = 0x00;
    response[offset++] = 0x00;
    response[offset++] = 0x3C; // TTL = 60s
    response[offset++] = 0x00;
    response[offset++] = 0x04; // RDLENGTH = 4

    // IP 转为4字节写入
    unsigned char ipParts[4];
    sscanf(ip, "%hhu.%hhu.%hhu.%hhu", &ipParts[0], &ipParts[1], &ipParts[2], &ipParts[3]);
    memcpy(response + offset, ipParts, 4);
    offset += 4;

    return offset;
}

