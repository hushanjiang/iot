
/*
请使用如下命令导入本文件
mysql -h127.0.0.1 -uxcp -pxcp < test.sql -f --default-character-set=utf8
*/

SET FOREIGN_KEY_CHECKS=0;

use hdiot_common;
delete from tbl_uuid;
insert into tbl_uuid(F_name, F_index, F_nr_peralloc, F_created_at) values('mdp_id', 10000, 1000, unix_timestamp());
insert into tbl_uuid(F_name, F_index, F_nr_peralloc, F_created_at) values('family_id', 1, 1000, unix_timestamp());

delete from tbl_key;
insert into tbl_key(F_name, F_value, F_created_at) values('auth_key', '123abc', unix_timestamp());
insert into tbl_key(F_name, F_value, F_created_at) values('security_channel_pub', '30819D300D06092A864886F70D010101050003818B0030818702818100AD9EBE9A7400DCA298D6B933CE99D508B25B1A24CB8C94F16B5E10C575973AE310906043F8578F350F2EAC102AC66C891CDD13DDA6A8402AEE9F4790190D2FA9C5366434926EB8CDFA2A06BB49428A53A6783282542A6282FD9144A15C1F5F545D8D040148F056AF9707974E5587B509E74D7913F3692969BD35BABF0593574B020111', unix_timestamp());
insert into tbl_key(F_name, F_value, F_created_at) values('security_channel_pri','30820274020100300D06092A864886F70D01010105000482025E3082025A02010002818100AD9EBE9A7400DCA298D6B933CE99D508B25B1A24CB8C94F16B5E10C575973AE310906043F8578F350F2EAC102AC66C891CDD13DDA6A8402AEE9F4790190D2FA9C5366434926EB8CDFA2A06BB49428A53A6783282542A6282FD9144A15C1F5F545D8D040148F056AF9707974E5587B509E74D7913F3692969BD35BABF0593574B02011102818023BECCE3905A87C71F77807B9B7A06367F12C19E29E83CC84ACF9A0A8926AA3DCEB4500DFE6C613FA13E5099EAB061A3C22D84170BB93A633120CAF805285CA29D15710D7A487B166BE362EE03257A4AE8FCB248C0D983F79C019669652FCFE072332991AD79E471761CE8B3CAF46B406C3E67CC17F6D50BF112A171945FC2F9024100D10FB1BD18D4B44CD4DB5DD85DB36334B018F0AD55696DD36994D9DB118103AF6502D4902F69E2B6D09B45185AD3315D493802CB344399CF9EC31EDE511C7923024100D499FB1170CC40EFAB907F3A49FDB08E8AB8722A7AA073B3C2D0227D3A918E876023F7D21811D53F64290C838EA39F4F229D34FBB876A9601DCEF96FE3CA4FB9024018986F436C55426364560B0A6560660632D5C1F64648A38248A819A14D5A78E7754BA0896EFD65F763D60820FBA0602917AC3C9060806C72C7623FDDEB6CC2F50241008990CFA1DF9339138D214343D58608D4B41CFE93F4FE68FBD8688EC9804010EE2F26550F78FC7AECC856CBDCA7970CBABC0B5E84C2A7224D4076DDA2C091F7590240430F674773AC6F1A4AFE6592D8A2C023F5FDAA7CE05685FABECF7912645D8748179673B8FDD98496ED2736CFC619B62B08AFE7990788C0DA0C5AD4FB4515EAE9', unix_timestamp());

delete from tbl_device_type;
insert into tbl_device_type(F_name, F_order, F_created_at) values('智能灯', 1, unix_timestamp());
insert into tbl_device_type(F_name, F_order, F_created_at) values('窗帘', 2, unix_timestamp());
insert into tbl_device_type(F_name, F_order, F_created_at) values('电视', 3, unix_timestamp());
insert into tbl_device_type(F_name, F_order, F_created_at) values('空调', 4, unix_timestamp());
insert into tbl_device_type(F_name, F_order, F_created_at) values('电饭煲', 5, unix_timestamp());

delete from tbl_svr_access;
insert into tbl_svr_access(F_svr, F_ip, F_port, F_created_at) values('lb_svr', '127.0.0.1', '2001', unix_timestamp());
insert into tbl_svr_access(F_svr, F_ip, F_port, F_created_at) values('user_mgt_svr', '127.0.0.1', '3000', unix_timestamp());
insert into tbl_svr_access(F_svr, F_ip, F_port, F_created_at) values('user_mgt_svr', '127.0.0.1', '3001', unix_timestamp());
insert into tbl_svr_access(F_svr, F_ip, F_port, F_created_at) values('device_mgt_svr', '127.0.0.1', '4000', unix_timestamp());
insert into tbl_svr_access(F_svr, F_ip, F_port, F_created_at) values('family_mgt_svr', '127.0.0.1', '5000', unix_timestamp());
insert into tbl_svr_access(F_svr, F_ip, F_port, F_created_at) values('uid_svr', '127.0.0.1', '6000', unix_timestamp());
insert into tbl_svr_access(F_svr, F_ip, F_port, F_created_at) values('security_svr', '127.0.0.1', '7000', unix_timestamp());
insert into tbl_svr_access(F_svr, F_ip, F_port, F_created_at) values('conf_svr', '127.0.0.1', '8100', unix_timestamp());
insert into tbl_svr_access(F_svr, F_ip, F_port, F_created_at) values('mdp', '127.0.0.1', '9000', unix_timestamp());

delete from tbl_white_list;
insert into tbl_white_list(F_svr, F_ip, F_created_at) values('lb_svr', '127.0.0.1', unix_timestamp());
insert into tbl_white_list(F_svr, F_ip, F_created_at) values('user_mgt_svr', '127.0.0.1', unix_timestamp());
insert into tbl_white_list(F_svr, F_ip, F_created_at) values('device_mgt_svr', '127.0.0.1', unix_timestamp());
insert into tbl_white_list(F_svr, F_ip, F_created_at) values('family_mgt_svr', '127.0.0.1', unix_timestamp());
insert into tbl_white_list(F_svr, F_ip, F_created_at) values('uid_svr', '127.0.0.1', unix_timestamp());
insert into tbl_white_list(F_svr, F_ip, F_created_at) values('mdp', '127.0.0.1', unix_timestamp());


delete from tbl_permission;
insert into tbl_permission(F_method, F_desc, F_created_at) values('create_family', 'create new family', unix_timestamp());
insert into tbl_permission(F_method, F_desc, F_created_at) values('update_family', 'update family', unix_timestamp());
insert into tbl_permission(F_method, F_desc, F_created_at) values('get_family_info', 'get family info', unix_timestamp());
insert into tbl_permission(F_method, F_desc, F_created_at) values('get_family_list', 'get family list', unix_timestamp());
insert into tbl_permission(F_method, F_desc, F_created_at) values('apply_family', 'apply family', unix_timestamp());
insert into tbl_permission(F_method, F_desc, F_created_at) values('accept_family', 'accept family', unix_timestamp());

delete from tbl_role;
insert into tbl_role(F_name, F_desc, F_created_at) values('owner', '户主', unix_timestamp());
insert into tbl_role(F_name, F_desc, F_created_at) values('member', '家庭成员', unix_timestamp());	

delete from tbl_role_permission;
insert into tbl_role_permission(F_role_id, F_permission_id, F_created_at) values(1, 1, unix_timestamp());
insert into tbl_role_permission(F_role_id, F_permission_id, F_created_at) values(1, 2, unix_timestamp());
