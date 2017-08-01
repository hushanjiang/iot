
#!/bin/bash

user=xcp
password=xcp
mysql_exec="/usr/bin/mysql -h127.0.0.1 -u$user -p$password"


##########################
#【公共数据库】
##########################
db=hdiot_common

$mysql_exec -e "drop database $db"
$mysql_exec -e "create database $db"


#-------------------------
#全局唯一id配置表
#-------------------------
echo "begin create ${db}.tbl_uuid table";		
$mysql_exec $db -e "CREATE TABLE ${db}.tbl_uuid(
		F_id int(6) unsigned AUTO_INCREMENT COMMENT '唯一ID编号',
		F_name varchar(100) COMMENT 'ID名称',
		F_index bigint(11) NOT NULL DEFAULT 0 COMMENT '当前分配的ID索引',
		F_nr_peralloc int(11) NOT NULL DEFAULT 1 COMMENT '每次更新数据库时预分配的数量',
		F_created_at bigint COMMENT '创建时间戳',
		F_last_update_at bigint COMMENT '上次更新时间戳',
		PRIMARY KEY (F_id),
		INDEX idx_name(F_name)
	) ENGINE=InnoDB DEFAULT  CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";


#-------------------------
#版本配置表
#-------------------------
echo "begin create ${db}.tbl_ver_config table";		
$mysql_exec $db -e "CREATE TABLE ${db}.tbl_ver_config(
		F_id int(6) unsigned AUTO_INCREMENT COMMENT '唯一ID编号',
		F_product_id int NOT NULL COMMENT '产品ID,',
		F_channel_id int NOT NULL COMMENT '渠道ID',		
		F_ver_id int NOT NULL COMMENT '版本ID',
		F_update_type int NOT NULL DEFAULT 2 COMMENT '2-可选、1-强制',
		F_update_action int NOT NULL DEFAULT 1 COMMENT '升级动作: 1-升级程序、2-升级数据',
		F_ver_desc  varchar(5000)  COMMENT '版本描述',
		F_ver_code  varchar(128)  COMMENT '版本编码',
		F_update_pkg_url  varchar(1024)  COMMENT '版本url',
		F_update_pkg_md5  varchar(1024)  COMMENT '版本md5',
		F_created_at bigint COMMENT '创建时间戳',
		PRIMARY KEY (F_id)
	) ENGINE=InnoDB DEFAULT  CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";	


	
#-------------------------
#秘钥配置表
#-------------------------
echo "begin create ${db}.tbl_key table";		
$mysql_exec $db -e "CREATE TABLE ${db}.tbl_key(
		F_id int(6) unsigned AUTO_INCREMENT COMMENT '唯一ID编号',
		F_name varchar(100) NOT NULL DEFAULT '' COMMENT '秘钥名称',
		F_value varchar(4096) COMMENT '秘钥信息',
		F_created_at bigint COMMENT '创建时间戳',
		F_updated_at bigint COMMENT '更新时间戳',
		F_expires_at bigint COMMENT '过期时间戳',
		PRIMARY KEY (F_id)
	) ENGINE=InnoDB DEFAULT  CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";


	
#-------------------------
#设备类型配置表
#-------------------------
echo "begin create ${db}.tbl_device_type table";		
$mysql_exec $db -e "CREATE TABLE ${db}.tbl_device_type(
		F_id int(6) unsigned AUTO_INCREMENT COMMENT '唯一ID编号',
		F_name varchar(100) NOT NULL DEFAULT '' COMMENT '设备类型名称',
		F_order int(11) NOT NULL DEFAULT 1 COMMENT '显示顺序',
		F_created_at bigint COMMENT '创建时间戳',
		F_updated_at bigint COMMENT '更新时间戳',
		PRIMARY KEY (F_id)
	) ENGINE=InnoDB DEFAULT  CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";


#-------------------------
#服务器访问配置表
#-------------------------
echo "begin create ${db}.tbl_svr_access table";		
$mysql_exec $db -e "CREATE TABLE ${db}.tbl_svr_access(
		F_id int(6) unsigned AUTO_INCREMENT COMMENT '唯一ID编号',
		F_svr varchar(100) NOT NULL DEFAULT '' COMMENT '服务类型名称',
		F_ip varchar(400) NOT NULL DEFAULT '127.0.0.1' COMMENT 'IP地址',
		F_port varchar(400) NOT NULL DEFAULT '' COMMENT '端口列表',
		F_created_at bigint COMMENT '创建时间戳',
		F_updated_at bigint COMMENT '更新时间戳',
		PRIMARY KEY (F_id),
		INDEX idx_svr(F_svr)
	) ENGINE=InnoDB DEFAULT  CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";



#-------------------------
#白名单访问配置表
#-------------------------
echo "begin create ${db}.tbl_white_list table";		
$mysql_exec $db -e "CREATE TABLE ${db}.tbl_white_list(
		F_id int(6) unsigned AUTO_INCREMENT COMMENT '唯一ID编号',
		F_svr varchar(100) NOT NULL DEFAULT '' COMMENT '服务类型名称',
		F_ip varchar(50) NOT NULL DEFAULT '127.0.0.1' COMMENT 'IP地址',
		F_created_at bigint COMMENT '创建时间戳',
		F_updated_at bigint COMMENT '更新时间戳',
		PRIMARY KEY (F_id),
		INDEX idx_svr(F_svr)
	) ENGINE=InnoDB DEFAULT  CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";



#-------------------------
#权限配置表
#-------------------------
echo "begin create ${db}.tbl_permission table";		
$mysql_exec $db -e "CREATE TABLE ${db}.tbl_permission(
		F_id int(6) unsigned AUTO_INCREMENT COMMENT '唯一ID编号',
		F_method varchar(100) NOT NULL DEFAULT '' COMMENT '方法名称',
		F_desc varchar(500) COMMENT '描述',
		F_created_at bigint COMMENT '创建时间戳',
		F_updated_at bigint COMMENT '更新时间戳',
		PRIMARY KEY (F_id)
	) ENGINE=InnoDB DEFAULT  CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";

	
	
#-------------------------
#角色配置表
#-------------------------
echo "begin create ${db}.tbl_role table";		
$mysql_exec $db -e "CREATE TABLE ${db}.tbl_role(
		F_id int(6) unsigned AUTO_INCREMENT COMMENT '唯一ID编号',
		F_name varchar(100) NOT NULL DEFAULT '' COMMENT '角色名称',
		F_desc varchar(500) COMMENT '角色描述',
		F_created_at bigint COMMENT '创建时间戳',
		F_updated_at bigint COMMENT '更新时间戳',
		PRIMARY KEY (F_id)
	) ENGINE=InnoDB DEFAULT  CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";



#-------------------------
#角色权限配置表
#-------------------------
echo "begin create ${db}.tbl_role_permission table";		
$mysql_exec $db -e "CREATE TABLE ${db}.tbl_role_permission(
		F_role_id int(6) unsigned COMMENT '角色ID',
		F_permission_id int(6) unsigned COMMENT '权限ID',
		F_created_at bigint COMMENT '创建时间戳',
		F_updated_at bigint COMMENT '更新时间戳',
		PRIMARY KEY (F_role_id, F_permission_id)
	) ENGINE=InnoDB DEFAULT  CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";
	
	

##########################
#【用户数据库】
##########################
db=hdiot_user_data

$mysql_exec -e "drop database $db"
$mysql_exec -e "create database $db"


#-------------------------
#安全通道表
#-------------------------
echo "begin create ${db}.tbl_security_channel table";	
$mysql_exec $db -e "CREATE TABLE ${db}.tbl_security_channel(
		F_id int unsigned AUTO_INCREMENT COMMENT '唯一ID编号',
		F_guid varchar(100) NOT NULL COMMENT '设备的guid，包括APP和路由器',
		F_key  varchar(255) NOT NULL COMMENT '客户端创建的安全通道key',
		F_created_at bigint COMMENT '创建时间戳',
		F_updated_at bigint COMMENT '更新时间戳',
		F_expires_at bigint COMMENT '过期时间戳',
		PRIMARY KEY (F_id),
		INDEX idx_guid(F_guid)
	) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";


#-------------------------
#短信验证码表
#-------------------------			
echo "begin create ${db}.tbl_user_verify_code table";					
$mysql_exec $db -e "CREATE TABLE ${db}.tbl_user_verify_code(
		F_id int unsigned AUTO_INCREMENT COMMENT '唯一ID编号',
		F_phone_num varchar(20) NOT NULL COMMENT '手机号',
		F_verify_code varchar(16) NOT NULL COMMENT '验证码',
		F_type smallint(6) NOT NULL COMMENT '类型：1-注册、2-登录、3-密码找回',
		F_content varchar(255)  NOT NULL DEFAULT '' COMMENT '短信内容',
		F_state tinyint(4) NOT NULL DEFAULT 0 COMMENT '状态：0-已经发送、1-已经使用',
		F_created_at bigint NOT NULL COMMENT '创建时间戳',
		F_expires_at bigint COMMENT '过期时间戳',
		PRIMARY KEY (F_id),
		INDEX idx_phone (F_phone_num)
	)ENGINE=InnoDB DEFAULT  CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";

	

#-------------------------
#用户登录映射表
#-------------------------			
echo "begin create ${db}.tbl_user_map table";					
$mysql_exec $db -e "CREATE TABLE ${db}.tbl_user_map(
		F_uid bigint NOT NULL COMMENT '用户内部ID',
		F_phone_num varchar(20) NOT NULL COMMENT '用户手机号',
		PRIMARY KEY (F_uid),
		UNIQUE KEY(F_phone_num)
	)ENGINE=InnoDB DEFAULT  CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";

	


#-------------------------
#用户相关数据表，分表
#-------------------------	
for i in {0..9}
do

	#用户表
	echo "begin create ${db}.tbl_user_info_${i}";	
	$mysql_exec $db -e "CREATE TABLE ${db}.tbl_user_info_${i}(
			F_uid bigint NOT NULL COMMENT '用户内部ID',
			F_phone_num varchar(20) NOT NULL COMMENT '用户手机号',
			F_password varchar(255) NOT NULL DEFAULT '' COMMENT '密码',
			F_state tinyint(4) NOT NULL DEFAULT 0 COMMENT '状态：0-未激活、1-激活、2-禁用',
			F_role_id int(6) NOT NULL DEFAULT 0 COMMENT '角色ID',
			F_nick varchar(100) NOT NULL DEFAULT '' COMMENT '昵称',
			F_name varchar(100) NOT NULL DEFAULT '' COMMENT '真实姓名',
			F_avatar varchar(255) NOT NULL DEFAULT '' COMMENT '头像Url',
			F_gender tinyint(4) NOT NULL DEFAULT 0 COMMENT '性别：0-未配置、1-男、2-女',				
			F_last_login_ip varchar(45) NOT NULL DEFAULT '' COMMENT '最近登录IP',
			F_last_login_time bigint COMMENT '最近登录建时间戳',
			F_created_at bigint COMMENT '创建时间戳',
			F_updated_at bigint COMMENT '更新时间戳',
			PRIMARY KEY (F_uid),
			UNIQUE KEY F_user_phone_unique(F_phone_num)
		) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";

			
	# 用户基本信息表		
	echo "begin create ${db}.tbl_user_profile tables";				
	$mysql_exec $db -e "CREATE TABLE ${db}.tbl_user_profile_${i}(
			F_uid bigint NOT NULL COMMENT '用户内部ID',			
			F_age date COMMENT '出生日期',
			F_provice varchar(64) COMMENT '用户的省份',
			F_city varchar(64) COMMENT '用户的城市',
			F_district varchar(128) COMMENT '用户的小区',
			F_email varchar(100) NOT NULL DEFAULT '' COMMENT '邮箱',			
			F_addr varchar(1024) COMMENT '用户的地址',
			F_qq  varchar(24) COMMENT 'qq号',
			F_weixin varchar(24) COMMENT 'weixin open id',			
			F_contacts_name varchar(128) COMMENT '联系人的名称',
			F_contacts_phone varchar(128) COMMENT '联系人手机',
			F_ext varchar(1024) NOT NULL  COMMENT '扩展字段',
			F_created_at bigint COMMENT '创建时间',
			F_updated_at bigint COMMENT '更新时间',	
			PRIMARY KEY (F_uid)
		) ENGINE=InnoDB DEFAULT  CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";
	

	#用户token表，相同用户不同设备可以同时登陆，具有不同的token	
	echo "begin create ${db}.tbl_access_token_${i}";	
	$mysql_exec $db -e "CREATE TABLE ${db}.tbl_access_token_${i}(
			F_id int unsigned AUTO_INCREMENT COMMENT '唯一ID编号',
			F_uid bigint NOT NULL COMMENT '用户内部ID',
			F_guid varchar(100) NOT NULL COMMENT '设备的guid，包括APP和路由器',
			F_token  varchar(255) NOT NULL COMMENT '用户在该设备的token',
			F_revoked tinyint(1) NOT NULL  COMMENT '0-未撤销、1-撤销',
			F_created_at bigint COMMENT '创建时间戳',
			F_updated_at bigint COMMENT '更新时间戳',
			F_expires_at bigint COMMENT '过期时间戳',
			PRIMARY KEY (F_id),
			INDEX idx_uid_gid (F_uid,F_guid)
		) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";


	#家庭表,按照familyid 分表
	echo "begin create ${db}.tbl_family_${i}";		
	$mysql_exec $db -e "CREATE TABLE tbl_family_${i}(
			F_family_id bigint NOT NULL COMMENT '家庭内部ID',
			F_name varchar(100) NOT NULL DEFAULT '' COMMENT '家庭名称',
			F_owner_id bigint NOT NULL DEFAULT 0 COMMENT '户主用户内部ID',
			F_route_id bigint NOT NULL DEFAULT 0 COMMENT '家庭绑定的路由器内部id',
			F_image varchar(255) COMMENT '家庭头像',
			F_provice varchar(64) COMMENT '省份',
			F_city varchar(64) COMMENT '城市',
			F_district varchar(128) COMMENT '小区',
			F_addr varchar(255)  COMMENT '地址',		
			F_building_no varchar(128) COMMENT '栋',
			F_room_no varchar(128) COMMENT '房号',
			F_created_at bigint COMMENT '创建时间戳',
			F_updated_at bigint COMMENT '更新时间戳',  
			PRIMARY KEY (F_family_id)
		) ENGINE=InnoDB  DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";		
			

	#申请加入家庭表	
	echo "begin create ${db}.tbl_join_apply_${i}";		
	$mysql_exec $db -e "CREATE TABLE tbl_join_apply_${i}(
			F_id int(10) unsigned NOT NULL AUTO_INCREMENT  COMMENT '唯一ID编号',
			F_uid bigint NOT NULL DEFAULT 0 COMMENT '用户内部ID',
			F_family_id bigint NOT NULL DEFAULT 0 COMMENT '家庭id',
			F_state tinyint(4) NOT NULL DEFAULT 0 COMMENT '状态：0-待审批、1-已通过、2-被拒绝',		  
			F_created_at bigint COMMENT '创建时间戳',
			F_updated_at bigint COMMENT '更新时间戳',
			PRIMARY KEY (F_id),
			INDEX idx_uid_fid (F_uid,F_family_id)
		) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";			
			
			

	#加入家庭二维码，按照familyid分表		
	echo "begin create ${db}.tbl_apply_code_${i}";		
	$mysql_exec $db -e "CREATE TABLE tbl_apply_code_${i}(
			F_id int(10) unsigned NOT NULL AUTO_INCREMENT  COMMENT '唯一ID编号',
			F_family_id bigint NOT NULL DEFAULT 0 COMMENT '家庭id',
			F_code varchar(255) NOT NULL DEFAULT '' COMMENT '二维码Url',
			F_state tinyint(4) NOT NULL DEFAULT 0 COMMENT '状态：1-有效、2-失效',		  
			F_created_at bigint COMMENT '创建时间戳',
			F_expires_at bigint COMMENT '过期时间戳',
			PRIMARY KEY (F_id),
			INDEX idx_fid (F_family_id)
		) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";	
			

	#家庭成员表，按照familyid 分表	
	echo "begin create ${db}.tbl_user_family_${i}";		
	$mysql_exec $db -e "CREATE TABLE tbl_user_family_${i}(
			F_id int(10) unsigned NOT NULL AUTO_INCREMENT  COMMENT '唯一ID编号',
			F_family_id bigint NOT NULL DEFAULT 0 COMMENT '家庭id',		  
			F_uid bigint NOT NULL DEFAULT 0 COMMENT '用户内部ID',
			F_memo varchar(100) NOT NULL DEFAULT '' COMMENT '家庭成员备注名称',
			F_role_id int(6) NOT NULL DEFAULT 0 COMMENT '角色ID',
			F_state tinyint(4) NOT NULL DEFAULT 1 COMMENT '状态：1启用，0禁用',
			F_created_at bigint COMMENT '创建时间戳',
			F_updated_at bigint COMMENT '更新时间戳', 
			PRIMARY KEY (F_id),
			INDEX idx_fid_uid (F_family_id,F_uid)
		) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";	

done



##########################
#【设备数据库】
##########################
db=hdiot_dev_data

$mysql_exec -e "drop database $db"
$mysql_exec -e "create database $db"


#-------------------------
#路由器登录映射表
#-------------------------			
echo "begin create ${db}.tbl_device_map table";					
$mysql_exec $db -e "CREATE TABLE ${db}.tbl_device_map(
		F_did bigint NOT NULL COMMENT '设备的内部ID',
		F_device_id  varchar(64) NOT NULL COMMENT '设备的标识号，印刷在设备上的，生产时设置',	
		PRIMARY KEY (F_did),
		UNIQUE KEY(F_device_id)
	)ENGINE=InnoDB DEFAULT  CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";

	
 
for i in {0..9}
do

	#-------------------------
	#房间表,按照familyid 分表
	#-------------------------
	echo "begin create ${db}.tbl_room_${i}";	
	$mysql_exec $db -e "CREATE TABLE ${db}.tbl_room_${i}(
			F_id int(10) unsigned NOT NULL AUTO_INCREMENT  COMMENT '唯一ID编号',
			F_family_id bigint NOT NULL DEFAULT 0 COMMENT '家庭id',	
			F_room_name varchar(255) NOT NULL DEFAULT '' COMMENT '房间名称',
			F_disp_order int(11) NOT NULL DEFAULT 1 COMMENT '房间显示顺序号，从1开始',	
			F_creator bigint NOT NULL DEFAULT 0 COMMENT '创建用户内部ID',
			F_created_at bigint COMMENT '创建时间戳',
			F_updated_at bigint COMMENT '更新时间戳',
			PRIMARY KEY (F_id),
			INDEX idx_fid(F_family_id),
			INDEX idx_creator(F_creator)
		) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";
			
	
	
	#-------------------------
	#设备信息表,按照did分表
	#-------------------------
	echo "begin create ${db}.tbl_device_info_${i}";	
	$mysql_exec $db -e "CREATE TABLE ${db}.tbl_device_info_${i} (
			F_did bigint NOT NULL DEFAULT 0 COMMENT '设备的内部ID',
			F_product_id bigint NOT NULL DEFAULT 0 COMMENT '产品id，tbl_product分配的id，唯一标识产品的一个产品种类',
			F_business_id bigint NOT NULL DEFAULT 0 COMMENT '设备厂家的企业内部id，开放平台tbl_business_user',
			F_device_name varchar(255) NOT NULL DEFAULT '' COMMENT '设备的名称',
			F_device_id   varchar(64) NOT NULL COMMENT '设备的标识号，印刷在设备上的，生产时设置',
			F_device_auth_key varchar(24) NOT NULL COMMENT '设备的鉴权码或者token-key',
			F_device_sn   varchar(64) NOT NULL COMMENT '设备的串号，生产时设置,保留',	
			F_device_imei varchar(64) NOT NULL COMMENT '设备的IMEI，预留',	
			F_device_mac  varchar(64) NOT NULL COMMENT '设备的mac地址',
			F_battery_percent float DEFAULT 0.0  COMMENT '设备的电池容量，保留',
			F_device_status  tinyint unsigned NOT NULL DEFAULT 2 COMMENT '设备的状态 1-在线,2-不在线',
			F_device_state tinyint unsigned NOT NULL DEFAULT 1 COMMENT '设备的情况，1-工厂测试,2-库存,3-销售出,5-返修,6-失效', 
			F_created_at bigint COMMENT '创建时间戳',
			F_updated_at bigint COMMENT '更新时间戳',
			PRIMARY KEY (F_did),
			INDEX idx_did(F_device_id),
			INDEX idx_status (F_device_status),
			INDEX idx_state(F_device_state)
		) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci"; 


			
	#-------------------------
	#房间设备表,按照familyid 分表
	#-------------------------
	echo "begin create ${db}.tbl_room_device_${i}";	
	$mysql_exec $db -e "CREATE TABLE ${db}.tbl_room_device_${i} (
			F_id int(10) unsigned NOT NULL AUTO_INCREMENT  COMMENT '唯一ID编号',
			F_family_id bigint NOT NULL DEFAULT 0 COMMENT '家庭id',	
			F_room_id int(11) NOT NULL DEFAULT 1 COMMENT '房间id',		
			F_device_type int unsigned NOT NULL DEFAULT 0 COMMENT '设备的类型',		   
			F_did bigint NOT NULL DEFAULT 0 COMMENT '设备的内部ID',
			F_device_name varchar(255) NOT NULL DEFAULT '' COMMENT '设备的名称',
			F_creator bigint NOT NULL DEFAULT 0 COMMENT '用户内部ID',
			F_created_at bigint COMMENT '创建时间戳',
			F_updated_at bigint COMMENT '更新时间戳',
			PRIMARY KEY (F_id),
			INDEX idx_fid_rid(F_family_id,F_room_id),
			INDEX idx_fid_rid_did (F_family_id, F_room_id, F_did)		   
		) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";  
			
			
			
	#-------------------------
	#设备报警表（按照did%9分表）
	#-------------------------
	echo "begin create  ${db}.t_device_alert_${i} tables";		
	$mysql_exec $db -e "CREATE TABLE ${db}.t_device_alert_${i}(
			F_id int(10) unsigned NOT NULL AUTO_INCREMENT  COMMENT '唯一ID编号',
			F_did bigint NOT NULL,
			F_alert_id int unsigned NOT NULL COMMENT '用户的报警ID',
			F_alert_type  tinyint unsigned NOT NULL DEFAULT 1 COMMENT '报警类型 1-',
			F_alert_msg  varchar(255) NOT NULL DEFAULT '' COMMENT '告警内容',
			F_alert_time datetime COMMENT '设备告警时间',
			F_alert_staus tinyint unsigned default 1 COMMENT '报警状态 1-通知用户 2-用户处理 ',
			F_created_at bigint COMMENT '创建时间戳',
			F_confirm_time bigint COMMENT '报警用户确认处理的时间',
			primary key(F_id),
			INDEX idx_did_aid(F_did, F_alert_id)
		) ENGINE=InnoDB DEFAULT CHARSET=utf8"
				
done


