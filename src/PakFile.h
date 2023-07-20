#pragma warning(disable: 4334)
#pragma warning(disable: 4267)

#pragma pack(push)
#pragma pack(1)

#define PACK4_FILE_VERSION  (4)
#define PACK5_FILE_VERSION  (5)

#include "brotli/brotli.h"

struct PAK4_HEADER
{
    uint32_t num_entries;
    uint8_t encodeing;
};

struct PAK5_HEADER
{
    uint32_t encodeing;
    uint16_t resource_count;
    uint16_t alias_count;
};

struct PAK_ENTRY
{
    uint16_t resource_id;
    uint32_t file_offset;
};

struct PAK_ALIAS
{
    uint16_t resource_id;
    uint16_t entry_index;
};
#pragma pack(pop)

uint32_t CheckHeader(uint8_t* buffer, PAK_ENTRY*& pak_entry, PAK_ENTRY*& end_entry)
{
    uint32_t version = *(uint32_t*)buffer;

    if (version != PACK4_FILE_VERSION && version != PACK5_FILE_VERSION) return 0;

    if (version == PACK4_FILE_VERSION)
    {
        PAK4_HEADER* pak_header = (PAK4_HEADER*)(buffer + sizeof(uint32_t));
        if (pak_header->encodeing != 1) return 0;

        pak_entry = (PAK_ENTRY*)(buffer + sizeof(uint32_t) + sizeof(PAK4_HEADER));
        end_entry = pak_entry + pak_header->num_entries;
    }

    if (version == PACK5_FILE_VERSION)
    {
        PAK5_HEADER* pak_header = (PAK5_HEADER*)(buffer + sizeof(uint32_t));
        if (pak_header->encodeing != 1) return 0;

        pak_entry = (PAK_ENTRY*)(buffer + sizeof(uint32_t) + sizeof(PAK5_HEADER));
        end_entry = pak_entry + pak_header->resource_count;
    }

    // 为了保存最后一条的"下一条"，这条特殊的条目的id一定为0
    if (!end_entry || end_entry->resource_id != 0) return 0;

    return end_entry->file_offset;
}

template<typename Function>
void PakFind(uint8_t* buffer, uint8_t* pos, Function f)
{
    PAK_ENTRY* pak_entry = NULL;
    PAK_ENTRY* end_entry = NULL;

    // 检查文件头
    if (!CheckHeader(buffer, pak_entry, end_entry)) return;

    do
    {
        PAK_ENTRY* next_entry = pak_entry + 1;
        if (pos >= buffer + pak_entry->file_offset && pos <= buffer + next_entry->file_offset)
        {
            f(buffer + pak_entry->file_offset, next_entry->file_offset - pak_entry->file_offset);
            break;
        }

        pak_entry = next_entry;
    } while (pak_entry->resource_id != 0);
}

template<typename Function>
void TraversalGZIPFile(uint8_t* buffer, Function f)
{
    PAK_ENTRY* pak_entry = NULL;
    PAK_ENTRY* end_entry = NULL;

    // 检查文件头
    if (!CheckHeader(buffer, pak_entry, end_entry)) return;

    do
    {
        PAK_ENTRY* next_entry = pak_entry + 1;
        uint32_t old_size = next_entry->file_offset - pak_entry->file_offset;

        if (old_size < 10 * 1024)
        {
            // 小于10k文件跳过
            pak_entry = next_entry;
            continue;
        }

        BYTE gzip[] = { 0x1F, 0x8B, 0x08 };
        size_t gzip_len = sizeof(gzip);
        if (memcmp(buffer + pak_entry->file_offset, gzip, gzip_len) != 0)
        {
            // 不是gzip文件跳过
            pak_entry = next_entry;
            continue;
        }

        uint32_t original_size = *(uint32_t*)(buffer + next_entry->file_offset - 4);
        uint8_t* unpack_buffer = (uint8_t*)malloc(original_size);
        if (!unpack_buffer) return;

        struct mini_gzip gz;
        mini_gz_start(&gz, buffer + pak_entry->file_offset, old_size);
        int unpack_len = mini_gz_unpack(&gz, unpack_buffer, original_size);

        if (original_size == unpack_len)
        {
            uint32_t new_len = old_size;
            bool changed = f(unpack_buffer, unpack_len, new_len);
            if (changed)
            {
                // 如果有改变
                size_t compress_size = 0;
                uint8_t* compress_buffer = (uint8_t*)gzip_compress(unpack_buffer, new_len, &compress_size);
                if (compress_buffer && compress_size < old_size)
                {
                    /*FILE *fp = fopen("test.gz", "wb");
                    fwrite(compress_buffer, compress_size, 1, fp);
                    fclose(fp);*/

                    //gzip头
                    memcpy(buffer + pak_entry->file_offset, compress_buffer, 10);

                    //extra
                    buffer[pak_entry->file_offset + 3] = 0x04;
                    uint16_t extra_length = old_size - compress_size - 2;
                    memcpy(buffer + pak_entry->file_offset + 10, &extra_length, sizeof(extra_length));
                    memset(buffer + pak_entry->file_offset + 12, '\0', extra_length);

                    //compress
                    memcpy(buffer + pak_entry->file_offset + 12 + extra_length, compress_buffer + 10, compress_size - 10);

                    /*fp = fopen("test2.gz", "wb");
                    fwrite(buffer + pak_entry->file_offset, old_size, 1, fp);
                    fclose(fp);*/
                }
                else
                {
                    DebugLog(L"gzip compress error %d %d", compress_size, old_size);
                }

                if (compress_buffer) free(compress_buffer);
            }
        }

        free(unpack_buffer);
        pak_entry = next_entry;
    } while (pak_entry->resource_id != 0);
}

template<typename Function>
void TraversalBrotliFile(uint8_t** buffer, Function f)
{
    PAK_ENTRY* pak_entry = NULL;
    PAK_ENTRY* end_entry = NULL;

    // 检查文件头
    uint32_t buffer_len = CheckHeader(*buffer, pak_entry, end_entry);
    if (!buffer_len) return;

    do
    {
        PAK_ENTRY* next_entry = pak_entry + 1;
        uint32_t old_size = next_entry->file_offset - pak_entry->file_offset;
        //plog(std::to_string(pak_entry->resource_id) + "  " + std::to_string(pak_entry->file_offset) + " | " + std::to_string(next_entry->file_offset) + "  " + std::to_string(next_entry->resource_id));
        //plog(std::to_string((uint8_t)pak_entry));
       
        if (old_size < 10 * 1024)
        {
            // 小于10k文件跳过
            pak_entry = next_entry;
            continue;
        }

        BYTE brotli[] = { 0x1E, 0x9B };
        size_t brotli_len = sizeof(brotli);
        uint8_t* pak_file = *buffer + pak_entry->file_offset;
        if (memcmp(pak_file, brotli, brotli_len) != 0)
        {
            // 不是Brotli文件跳过
            pak_entry = next_entry;
            continue;
        }

        size_t size_in = old_size - 8;
        size_t size_out = 1 << 25;
        uint8_t* buffer_in = pak_file + 8;
        uint8_t* buffer_out = (uint8_t*)malloc(size_out);
        if (!buffer_in || !buffer_out) return;

        BrotliDecompress(size_in, &size_out, buffer_in, buffer_out);

        //bool changed = f(buffer_out, size_out, size_in);
        //if (changed)
        //{
        //    memcpy(buffer_out, buffer_in, 8);
        //    plog(std::to_string(old_size) + "  " + std::to_string(size_in));
        //}

        std::string html((char*)buffer_out, size_out);

        if (html.find("aboutObsoleteEndOfTheLine") < std::string::npos)
        {
            //size_t size_new=0;
            //uint8_t* buffer_new = (uint8_t*)malloc(size_out);
            //memcpy(buffer_new, buffer_out, size_out);
            //BrotliCompress(size_out, &size_new, buffer_out, buffer_new);
            //memcpy(buffer_in, buffer_new, size_new);

            //ReplaceStringInPlace(html, R"(this.props.getSearchableLocalizedString("aboutProductTitle")),)", R"(""),)");
            //ReplaceStringInPlace(html, R"({className:this.props.managedClasses.aboutSection_footer_paragraph},this.props.getSearchableLocalizedString("aboutProductTitle"))", R"({className:this.props.managedClasses.aboutSection_footer_paragraph},""),)");

            //ReplaceStringInPlace(html, R"()", R"()");
            ReplaceStringInPlace(html, R"(e=this.props.currentUpdateStatusEvent.status)", R"(e="updated")");
            ReplaceStringInPlace(html, R"(renderIcon(){const e=this.props.currentUpdateStatusEvent;)", R"(renderIcon(){const e={"status":"updated"};)");
            ReplaceStringInPlace(html, R"(renderMainText(){if(this.isAppEndOfLine)return this.renderEndOfLineMessage();)", R"(renderMainText(){this.props.currentUpdateStatusEvent.status="updated";)");
            ReplaceStringInPlace(html, R"(this.props.getSearchableLocalizedString("aboutProductTitle")),)", R"("edge++ 1.5.1 inside"),)");

            uint8_t* buffer_new = (uint8_t*)malloc(buffer_len);
            uint8_t* buffer_new1 = (uint8_t*)malloc(buffer_len);
            if (buffer_new)
            {
                memcpy(buffer_new, *buffer, buffer_len);
                memcpy(buffer_new1, buffer_new, buffer_len);
                free(buffer_new);
                *buffer = buffer_new1;
            }

            //html = std::string("12345676");
            //BrotliCompress(html.length(), &size_out, (uint8_t*)&html[0], buffer_out+8);
            //size_out = size_out + 8;
            //memcpy(buffer_out, pak_file, 8);
            //memcpy(pak_file, buffer_out, size_out);
            //memcpy(pak_file+size_out, 0x00, old_size-size_out-10);
            
            //plog(std::to_string(old_size) + "  " + std::to_string(size_out));

        }

        free(buffer_out);
        pak_entry = next_entry;
    } while (pak_entry->resource_id != 0);
}

using namespace std;
void TraversalPakFile(uint8_t* buffer, uint8_t** nbuffer)
{
    PAK_ENTRY* top_entry = NULL;
    PAK_ENTRY* end_entry = NULL;

    // 检查文件头
    uint32_t buffer_len = CheckHeader(buffer, top_entry, end_entry);
    if (!buffer_len) return;

    int index = 0;
    *nbuffer = (uint8_t*)malloc(buffer_len);
    if (*nbuffer) memcpy(*nbuffer, buffer, buffer_len);
    buffer_len = CheckHeader(*nbuffer, top_entry, end_entry);

    do
    {
        PAK_ENTRY* pak_entry = top_entry + index;
        PAK_ENTRY* next_entry = top_entry + index + 1;
        uint32_t old_size = next_entry->file_offset - pak_entry->file_offset;

        if (old_size < 10 * 1024)
        {
            // 小于10k文件跳过
            index++;
            continue;
        }

        BYTE brotli[] = { 0x1E, 0x9B };
        size_t brotli_len = sizeof(brotli);
        uint8_t* pak_file = *nbuffer + pak_entry->file_offset;
        if (memcmp(pak_file, brotli, brotli_len) != 0)
        {
            // 不是Brotli文件跳过
            index++;
            continue;
        }

        size_t size_in = old_size - 8;
        size_t size_out = 1 << 24;
        uint8_t* buffer_in = pak_file + 8;
        uint8_t* buffer_out = (uint8_t*)malloc(size_out);
        if (!buffer_in || !buffer_out) return;

        BrotliDecompress(size_in, &size_out, buffer_in, buffer_out);

        std::string html((char*)buffer_out, size_out);

        if (html.find("aboutObsoleteEndOfTheLine") < std::string::npos)
        {
            
            ReplaceStringInPlace(html, R"(e=this.props.currentUpdateStatusEvent.status)", R"(e="updated")");
            ReplaceStringInPlace(html, R"(renderIcon(){const e=this.props.currentUpdateStatusEvent;)", R"(renderIcon(){const e={"status":"updated"};)");
            ReplaceStringInPlace(html, R"(renderMainText(){if(this.isAppEndOfLine)return this.renderEndOfLineMessage();)", R"(renderMainText(){this.props.currentUpdateStatusEvent.status="updated";)");
            ReplaceStringInPlace(html, R"(this.props.getSearchableLocalizedString("aboutProductTitle")),)", R"("iEdge © inside"),)");

            size_out = html.length();
            *buffer_out = 0x1E; *(buffer_out + 1) = 0x9B;
            memcpy(buffer_out + 2, &size_out, 6);

            BrotliCompress(html.length(), &size_out, (uint8_t*)&html[0], buffer_out + 8);
            size_out = size_out + 8;

            int size_diff = size_out - old_size;
            size_t size_new = buffer_len + size_diff;
            plog(to_string(size_diff));

            uint8_t* buffer_new = (uint8_t*)malloc(size_new);
            if (buffer_new)
            {

                memcpy(buffer_new, *nbuffer, pak_entry->file_offset);
                memcpy(buffer_new + pak_entry->file_offset, buffer_out, size_out);
                memcpy(buffer_new + pak_entry->file_offset + size_out, *nbuffer + next_entry->file_offset, buffer_len - next_entry->file_offset);

                //memcpy(buffer_new, *nbuffer, buffer_len);
                //memset(buffer_new + 1000, 0, 100);

                if (*nbuffer) free(*nbuffer);
                *nbuffer = buffer_new;
                CheckHeader(*nbuffer, top_entry, end_entry);
                PAK_ENTRY* fix_entry = top_entry + index + 1;
                for (;;)
                {
                    fix_entry->file_offset += size_diff;
                    
                    if (fix_entry->resource_id == 0)
                        break;
                    fix_entry += 1;
                }
                buffer_len = CheckHeader(*nbuffer, top_entry, end_entry);
            }
        }

        index++;
        free(buffer_out);
    } while ((top_entry+index)->resource_id != 0);
}
