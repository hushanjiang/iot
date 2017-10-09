
#include "mongo_mgt.h"
#include <libbson-1.0/bson.h>
#include <libmongoc-1.0/mongoc.h>
#include "base/base_string.h"
#include "base/base_logger.h"
#include "protocol.h"


extern Logger g_logger;

const unsigned int MAX_DEVICE_CNT_PER_DB	= 100;

Mongo_Mgt::Mongo_Mgt()
{

}


Mongo_Mgt::~Mongo_Mgt()
{

}

void Mongo_Mgt::update(StMongo_Access stMongo_Access)
{
	_stMongo_Access = stMongo_Access;
}


int Mongo_Mgt::storage(StDeviceStatusMsg &msg)
{
	XCP_LOGGER_INFO(&g_logger, "insert new status:%s.\n", msg._msg.c_str());
	return _queue.push(msg);
}



//db.tbl_session_0.find({ "$query" : { "session_id" : 5, "time" : { "$gte" : 1444060800, "$lte" : 1444406400 } }, "$orderby" : { "message_id" : -1 } }).skip(10).limit(2)
int Mongo_Mgt::get_device_status_message(const unsigned long long familyId, const unsigned long long deviceId, const std::string &date,
	const unsigned int skip, const unsigned int limit, std::deque<std::string> &msg_list, long long &total, std::string &err_info)
{
	int nRet = 0;
	err_info = "";

	/*
	XCP_LOGGER_INFO(&g_logger, "get_session_message, user id:%llu, session id:%llu, message id:%d, start time:%s, end time:%s, skip:%u, limit:%u\n", 
		user_id, session_id, message_id, start_date.c_str(), end_date.c_str(), skip, limit);
	*/
	
	bson_error_t error;
	mongoc_init ();

	mongoc_client_t *client = mongoc_client_new(_stMongo_Access._server.c_str());
	if(!client)
	{
		err_info = "mongoc_client_new failed.";
		XCP_LOGGER_INFO(&g_logger, "mongoc_client_new(%s) failed.\n", _stMongo_Access._server.c_str());
		return -1;
	}

	std::string db_name = format("%s_%s", _stMongo_Access._db.c_str(), date.c_str());
	//session id 一万一个collection
	std::string collection_name = format("%s_%u", _stMongo_Access._collection.c_str(), deviceId % MAX_DEVICE_CNT_PER_DB);
	mongoc_collection_t *collection = mongoc_client_get_collection(client, db_name.c_str(), collection_name.c_str());
	if(!collection)
	{
		XCP_LOGGER_INFO(&g_logger, "mongoc_client_new(db:%s, collection:%s) failed.\n", db_name.c_str(), collection_name.c_str());
		mongoc_client_destroy (client);
		return -2;
	}
	XCP_LOGGER_INFO(&g_logger, "mongoc_client_new(db:%s, collection:%s) success.\n", db_name.c_str(), collection_name.c_str());

	//组合查询bson
	bson_t filter;
	bson_init(&filter);
	
	//(1)$query
	bson_t query;
	bson_append_document_begin(&filter, "$query", -1, &query);
	
	//user_id
	//BSON_APPEND_INT64(&query, "user_id", user_id);   //不按照用户ID 过滤
	
	//session_id
	BSON_APPEND_INT64(&query, "family_id", familyId);
	BSON_APPEND_INT64(&query, "device_id", deviceId);
	
	bson_append_document_end(&filter, &query);
	
	
	//(3)$orderby
	bson_t orderby;
	bson_append_document_begin(&filter, "$orderby", -1, &orderby);
	BSON_APPEND_INT32(&orderby, "report_created_at", -1);
	bson_append_document_end(&filter, &orderby);

	char *str = bson_as_json(&filter, NULL);
	XCP_LOGGER_INFO(&g_logger, "cmd of mongo:%s\n", str);
	
	//获取符合要求的结果集
	mongoc_cursor_t *cursor = NULL;
    cursor = mongoc_collection_find (collection, MONGOC_QUERY_NONE, skip, limit, 0, &filter, NULL, NULL);
	if(!cursor)
	{
		err_info = "mongoc_collection_find failed.";
        XCP_LOGGER_INFO(&g_logger, "mongoc_collection_find failed.\n");
		mongoc_collection_destroy (collection);
		mongoc_client_destroy (client); 	
		return -1;		
	}

	const bson_t *doc = NULL;
	while (mongoc_cursor_next (cursor, &doc)) 
	{
		char *str = bson_as_json(doc, NULL);
		msg_list.push_back(str);
		bson_free(str);
	}

	total = msg_list.size();
		
	mongoc_cursor_destroy (cursor);    
	mongoc_collection_destroy (collection);
	mongoc_client_destroy (client); 
	
	return nRet;
	
}

int Mongo_Mgt::svc()
{
	int nRet = 0;
	
	//获取req 并且处理
	StDeviceStatusMsg msg;
	nRet = _queue.pop(msg);
	if(nRet != 0)
	{
		XCP_LOGGER_INFO(&g_logger, "mongoc_client_new(%s) failed.\n", _stMongo_Access._server.c_str());
		return 0;
	}
	
	mongoc_init ();
	mongoc_client_t *client = mongoc_client_new(_stMongo_Access._server.c_str());
	if(!client)
	{
		XCP_LOGGER_INFO(&g_logger, "mongoc_client_new(%s) failed.\n", _stMongo_Access._server.c_str());
		return -1;
	}

	XCP_LOGGER_INFO(&g_logger, "prepare to insert mongoc, device id:%llu, data:%s, msg:%s\n", msg._id, msg._date.c_str(), msg._msg.c_str());

	//session id 十万一个db
	std::string db_name = format("%s_%s", _stMongo_Access._db.c_str(), msg._date.c_str());
	//session id 一万一个collection
	std::string collection_name = format("%s_%u", _stMongo_Access._collection.c_str(), msg._id % MAX_DEVICE_CNT_PER_DB);
	mongoc_collection_t *collection = mongoc_client_get_collection(client, db_name.c_str(), collection_name.c_str());
	if(!collection)
	{
		XCP_LOGGER_INFO(&g_logger, "mongoc_client_new(db:%s, collection:%s) failed.\n", db_name.c_str(), collection_name.c_str());
		mongoc_client_destroy (client);
		return -2;
	}

	//todo: 创建索引

	bson_error_t error;
	bson_t *doc = bson_new_from_json ((const uint8_t *)msg._msg.c_str(), msg._msg.size(), &error);
	if(!doc)
	{
		XCP_LOGGER_INFO(&g_logger, "bson_new_from_json failed, err:%s, req(%u):%s\n", 
			error.message, msg._msg.size(), msg._msg.c_str());
		mongoc_collection_destroy (collection);
		mongoc_client_destroy (client); 		
		return -3;
	}

	//在执行命令的时候才和mondod建立连接
	if (!mongoc_collection_insert (collection, MONGOC_INSERT_NONE, doc, NULL, &error)) 
	{
		XCP_LOGGER_INFO(&g_logger, "mongoc_collection_insert failed, server:%s, db:%s, collection:%s, err:%s, req(%u):%s\n", 
			_stMongo_Access._server.c_str(), db_name.c_str(), collection_name.c_str(), 
			error.message, msg._msg.size(), msg._msg.c_str());
		nRet = -4;
	}
	else
	{
		XCP_LOGGER_INFO(&g_logger, "complete to insert mongoc, device id:%llu\n", msg._id);
	}

	bson_destroy (doc);
	mongoc_collection_destroy (collection);
	mongoc_client_destroy (client); 

	return 0;
	
}


int Mongo_Mgt::do_init(void *args)
{
	int nRet = 0;

	return nRet;
}

