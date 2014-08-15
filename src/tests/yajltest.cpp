#include "../test.h"
#include <cstdio>

extern "C" {
#include "yajl/yajl_common.h"
#undef YAJL_MAX_DEPTH 
#define YAJL_MAX_DEPTH 1024
#include "yajl/yajl_gen.h"
#include "yajl/yajl_parse.h"
#include "yajl/yajl_tree.h"

yajl_gen_status GenVal(yajl_gen g, yajl_val v) {
    yajl_gen_status status;
    switch (v->type) {
    case yajl_t_string: return yajl_gen_string(g, (unsigned char*)v->u.string, strlen(v->u.string));

    case yajl_t_number: 
        {
            char buffer[100];
            char *num = buffer;
            size_t len;
            //if (YAJL_IS_INTEGER(v)) // buggy
            if (v->u.number.flags & YAJL_NUMBER_INT_VALID)
#if _MSC_VER
                len = sprintf(num, "%I64d", YAJL_GET_INTEGER(v));
#else
                len = sprintf(num, "%lld", YAJL_GET_INTEGER(v));
#endif
            //else if (YAJL_IS_DOUBLE(v))   // buggy
            else if (v->u.number.flags & YAJL_NUMBER_DOUBLE_VALID)
                len = sprintf(num, "%g", YAJL_GET_DOUBLE(v));
            else {
                num = YAJL_GET_NUMBER(v);
                len = strlen(buffer);
            }
            return yajl_gen_number(g, num, len);
        }

    case yajl_t_object:
        status = yajl_gen_map_open(g);
        if (status != yajl_gen_status_ok)
            return status;
        
        for (size_t i = 0; i < v->u.object.len; i++) {
            status = yajl_gen_string(g, (unsigned char *)v->u.object.keys[i], strlen(v->u.object.keys[i]));
            if (status != yajl_gen_status_ok)
                return status;
            status = GenVal(g, v->u.object.values[i]);
            if (status != yajl_gen_status_ok)
                return status;
        }
        return yajl_gen_map_close(g);

    case yajl_t_array:
        status = yajl_gen_array_open(g);
        if (status != yajl_gen_status_ok)
            return status;
        
        for (size_t i = 0; i < v->u.array.len; i++) {
            status = GenVal(g, v->u.array.values[i]);
            if (status != yajl_gen_status_ok)
                return status;
        }

        return yajl_gen_array_close(g);

    case yajl_t_true: return yajl_gen_bool(g, 1);
    case yajl_t_false: return yajl_gen_bool(g, 0);
    case yajl_t_null: return yajl_gen_null(g);
    default:
        return yajl_gen_in_error_state;
    }
}

} // extern "C"

class YajlTest : public TestBase {
public:
	YajlTest() : TestBase("YAJL") {
	}
	
    virtual void* Parse(const char* json) const {
        yajl_val root = yajl_tree_parse(json, NULL, 0);
    	return root;
    }

    virtual char* Stringify(void* userdata) const {
        yajl_val root = (yajl_val)userdata;

        yajl_gen g = yajl_gen_alloc(NULL);
        yajl_gen_status status = GenVal(g, root);
        if (status != yajl_gen_status_ok) {
            printf("yajl Strinify error %d\n", status);
            return 0;
        }

        const unsigned char * buf;
        size_t len;
        status = yajl_gen_get_buf(g, &buf, &len);
        char* json = strdup((const char*)buf);

        yajl_gen_free(g);
        return json;
    }

    virtual char* Prettify(void* userdata) const {
        yajl_val root = (yajl_val)userdata;

        yajl_gen g = yajl_gen_alloc(NULL);
        yajl_gen_config(g, yajl_gen_beautify, 1);
        yajl_gen_config(g, yajl_gen_indent_string, "    ");

        yajl_gen_status status = GenVal(g, root);
        if (status != yajl_gen_status_ok)
            return 0;

        const unsigned char * buf;
        size_t len;
        status = yajl_gen_get_buf(g, &buf, &len);
        char* json = strdup((const char*)buf);

        yajl_gen_free(g);
        return json;
    }

    virtual void Free(void* userdata) const {
        yajl_val root = (yajl_val)userdata;
    	yajl_tree_free(root);
    }
};

REGISTER_TEST(YajlTest);
