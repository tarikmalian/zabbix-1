/*
** Zabbix
** Copyright (C) 2001-2021 Zabbix SIA
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**/

#include "dbcache.h"

#include "log.h"
#include "audit_graph.h"

static int	graph_flag_to_resource_type(int flag)
{
	if (ZBX_FLAG_DISCOVERY_NORMAL == flag || ZBX_FLAG_DISCOVERY_CREATED == flag)
	{
		return AUDIT_RESOURCE_GRAPH;
	}
	else if (ZBX_FLAG_DISCOVERY_PROTOTYPE == flag)
	{
		return AUDIT_RESOURCE_GRAPH_PROTOTYPE;
	}
	else
	{
		zabbix_log(LOG_LEVEL_CRIT, "unexpected audit graph flag detected: ->%d<-", flag);
		THIS_SHOULD_NEVER_HAPPEN;
		exit(EXIT_FAILURE);
	}
}
#define GR_OR_GRP(s) (AUDIT_RESOURCE_GRAPH == resource_type) ? "graph."#s : "graphprototype."#s

void	zbx_audit_graph_create_entry(int audit_action, zbx_uint64_t graphid, const char *name, int flags)
{
	int			resource_type;
	zbx_audit_entry_t	local_audit_graph_entry, **found_audit_graph_entry;
	zbx_audit_entry_t	*local_audit_graph_entry_x = &local_audit_graph_entry;

	RETURN_IF_AUDIT_OFF();

	resource_type = graph_flag_to_resource_type(flags);

	local_audit_graph_entry.id = graphid;

	found_audit_graph_entry = (zbx_audit_entry_t**)zbx_hashset_search(zbx_get_audit_hashset(),
			&(local_audit_graph_entry_x));

	if (NULL == found_audit_graph_entry)
	{
		zbx_audit_entry_t	*local_audit_graph_entry_insert;

		local_audit_graph_entry_insert = zbx_audit_entry_init(graphid, name, audit_action, resource_type);

		zbx_hashset_insert(zbx_get_audit_hashset(), &local_audit_graph_entry_insert,
				sizeof(local_audit_graph_entry_insert));

		if (AUDIT_ACTION_ADD == audit_action)
		{
			zbx_audit_update_json_append_uint64(graphid, AUDIT_DETAILS_ACTION_ADD, GR_OR_GRP(graphid),
					graphid);
		}
	}
}

void	zbx_audit_graph_update_json_add_data(zbx_uint64_t graphid, const char *graph_copy_name, int width, int height,
		double yaxismin, double yaxismax, zbx_uint64_t templateid, int show_work_period, int show_triggers,
		int graphtype, int show_legend, int show_3d, double percent_left, double percent_right, int ymin_type,
		int ymax_type, zbx_uint64_t ymin_itemid, zbx_uint64_t ymax_itemid, int flags, int discover)
{
	char	audit_key[AUDIT_DETAILS_KEY_LEN], audit_key_graph_copy_name[AUDIT_DETAILS_KEY_LEN],
		audit_key_width[AUDIT_DETAILS_KEY_LEN], audit_key_height[AUDIT_DETAILS_KEY_LEN],
		audit_key_yaxismin[AUDIT_DETAILS_KEY_LEN], audit_key_yaxismax[AUDIT_DETAILS_KEY_LEN],
		audit_key_templateid[AUDIT_DETAILS_KEY_LEN], audit_key_show_work_period[AUDIT_DETAILS_KEY_LEN],
		audit_key_show_triggers[AUDIT_DETAILS_KEY_LEN], audit_key_graphtype[AUDIT_DETAILS_KEY_LEN],
		audit_key_show_legend[AUDIT_DETAILS_KEY_LEN], audit_key_show_3d[AUDIT_DETAILS_KEY_LEN],
		audit_key_percent_left[AUDIT_DETAILS_KEY_LEN], audit_key_percent_right[AUDIT_DETAILS_KEY_LEN],
		audit_key_ymin_type[AUDIT_DETAILS_KEY_LEN], audit_key_ymax_type[AUDIT_DETAILS_KEY_LEN],
		audit_key_ymin_itemid[AUDIT_DETAILS_KEY_LEN], audit_key_ymax_itemid[AUDIT_DETAILS_KEY_LEN],
		audit_key_flags[AUDIT_DETAILS_KEY_LEN], audit_key_discover[AUDIT_DETAILS_KEY_LEN];
	int	resource_type;

	RETURN_IF_AUDIT_OFF();

	resource_type = graph_flag_to_resource_type(flags);

	zbx_snprintf(audit_key, sizeof(audit_key), (AUDIT_RESOURCE_GRAPH == resource_type) ? "graph" :
			"graphprototype");
#define AUDIT_KEY_SNPRINTF(r) zbx_snprintf(audit_key_##r, sizeof(audit_key_##r), GR_OR_GRP(r));
	AUDIT_KEY_SNPRINTF(graph_copy_name)
	AUDIT_KEY_SNPRINTF(width)
	AUDIT_KEY_SNPRINTF(height)
	AUDIT_KEY_SNPRINTF(yaxismin)
	AUDIT_KEY_SNPRINTF(yaxismax)
	AUDIT_KEY_SNPRINTF(templateid)
	AUDIT_KEY_SNPRINTF(show_work_period)
	AUDIT_KEY_SNPRINTF(show_triggers)
	AUDIT_KEY_SNPRINTF(graphtype)
	AUDIT_KEY_SNPRINTF(show_legend)
	AUDIT_KEY_SNPRINTF(show_3d)
	AUDIT_KEY_SNPRINTF(percent_left)
	AUDIT_KEY_SNPRINTF(percent_right)
	AUDIT_KEY_SNPRINTF(ymin_type)
	AUDIT_KEY_SNPRINTF(ymax_type)
	AUDIT_KEY_SNPRINTF(ymin_itemid)
	AUDIT_KEY_SNPRINTF(ymax_itemid)
	AUDIT_KEY_SNPRINTF(flags)
	AUDIT_KEY_SNPRINTF(discover)
#undef AUDIT_KEY_SNPRINTF
	zbx_audit_update_json_append_no_value(graphid, AUDIT_DETAILS_ACTION_ADD, audit_key);
#define ADD_STR(r) zbx_audit_update_json_append_string(graphid, AUDIT_DETAILS_ACTION_ADD, audit_key_##r, r);
#define ADD_UINT64(r) zbx_audit_update_json_append_uint64(graphid, AUDIT_DETAILS_ACTION_ADD, audit_key_##r, r);
#define ADD_INT(r) zbx_audit_update_json_append_int(graphid, AUDIT_DETAILS_ACTION_ADD, audit_key_##r, r);
#define ADD_DOUBLE(r) zbx_audit_update_json_append_double(graphid, AUDIT_DETAILS_ACTION_ADD, audit_key_##r, r);
	ADD_STR(graph_copy_name)
	ADD_INT(width)
	ADD_INT(height)
	ADD_DOUBLE(yaxismin)
	ADD_DOUBLE(yaxismax)
	ADD_UINT64(templateid)
	ADD_INT(show_work_period)
	ADD_INT(show_triggers)
	ADD_INT(graphtype)
	ADD_INT(show_legend)
	ADD_INT(show_3d)
	ADD_DOUBLE(percent_left)
	ADD_DOUBLE(percent_right)
	ADD_INT(ymin_type)
	ADD_INT(ymax_type)
	ADD_UINT64(ymin_itemid)
	ADD_UINT64(ymax_itemid)
	ADD_INT(flags)
	ADD_INT(discover)
#undef ADD_STR
#undef ADD_UINT64
#undef ADD_INT
#undef ADD_DOUBLE
}

void	zbx_audit_graph_update_json_add_gitems(zbx_uint64_t graphid, int flags, zbx_uint64_t gitemid, int drawtype,
		int sortorder, const char *color, int yaxisside, int calc_fnc, int type)
{
	char	audit_key_[AUDIT_DETAILS_KEY_LEN], audit_key_drawtype[AUDIT_DETAILS_KEY_LEN],
		audit_key_sortorder[AUDIT_DETAILS_KEY_LEN],
		audit_key_color[AUDIT_DETAILS_KEY_LEN], audit_key_yaxisside[AUDIT_DETAILS_KEY_LEN],
		audit_key_calc_fnc[AUDIT_DETAILS_KEY_LEN], audit_key_type[AUDIT_DETAILS_KEY_LEN];
	int	resource_type;

	RETURN_IF_AUDIT_OFF();

	resource_type = graph_flag_to_resource_type(flags);

#define AUDIT_KEY_GITEMS_SNPRINTF(r, nested) zbx_snprintf(audit_key_##r, sizeof(audit_key_##r),			\
		((AUDIT_RESOURCE_GRAPH == resource_type) ? "graph.gitems[" ZBX_FS_UI64 "]"#nested#r :		\
		"graphprototype.gitems[" ZBX_FS_UI64 "]"#nested#r), gitemid);
	AUDIT_KEY_GITEMS_SNPRINTF(,)
	AUDIT_KEY_GITEMS_SNPRINTF(drawtype, .)
	AUDIT_KEY_GITEMS_SNPRINTF(sortorder, .)
	AUDIT_KEY_GITEMS_SNPRINTF(color, .)
	AUDIT_KEY_GITEMS_SNPRINTF(yaxisside, .)
	AUDIT_KEY_GITEMS_SNPRINTF(calc_fnc, .)
	AUDIT_KEY_GITEMS_SNPRINTF(type, .)

	zbx_audit_update_json_append_no_value(graphid, AUDIT_DETAILS_ACTION_ADD, audit_key_);
#define ADD_STR(r) zbx_audit_update_json_append_string(graphid, AUDIT_DETAILS_ACTION_ADD, audit_key_##r, r);
#define ADD_INT(r) zbx_audit_update_json_append_int(graphid, AUDIT_DETAILS_ACTION_ADD, audit_key_##r, r);
	ADD_INT(drawtype)
	ADD_INT(sortorder)
	ADD_STR(color);
	ADD_INT(yaxisside)
	ADD_INT(calc_fnc)
	ADD_INT(type)
#undef ADD_STR
#undef ADD_INT
}

#define PREPARE_AUDIT_GRAPH_UPDATE(resource, type1, type2)							\
void	zbx_audit_graph_update_json_update_##resource(zbx_uint64_t graphid, int flags,				\
		type1 resource##_old, type1 resource##_new)							\
{														\
	char	buf[AUDIT_DETAILS_KEY_LEN];									\
	int	resource_type;											\
														\
	RETURN_IF_AUDIT_OFF();											\
														\
	resource_type = graph_flag_to_resource_type(flags);							\
														\
	zbx_snprintf(buf, sizeof(buf), GR_OR_GRP(resource));							\
														\
	zbx_audit_update_json_update_##type2(graphid, buf, resource##_old, resource##_new);			\
}

PREPARE_AUDIT_GRAPH_UPDATE(name, const char*, string)
PREPARE_AUDIT_GRAPH_UPDATE(width, int, int)
PREPARE_AUDIT_GRAPH_UPDATE(height, int, int)
PREPARE_AUDIT_GRAPH_UPDATE(yaxismin, double, double)
PREPARE_AUDIT_GRAPH_UPDATE(yaxismax, double, double)
PREPARE_AUDIT_GRAPH_UPDATE(show_work_period, int, int)
PREPARE_AUDIT_GRAPH_UPDATE(show_triggers, int, int)
PREPARE_AUDIT_GRAPH_UPDATE(graphtype, int, int)
PREPARE_AUDIT_GRAPH_UPDATE(show_legend, int, int)
PREPARE_AUDIT_GRAPH_UPDATE(show_3d, int, int)
PREPARE_AUDIT_GRAPH_UPDATE(percent_left, double, double)
PREPARE_AUDIT_GRAPH_UPDATE(percent_right, double, double)
PREPARE_AUDIT_GRAPH_UPDATE(ymin_type, int, int)
PREPARE_AUDIT_GRAPH_UPDATE(ymax_type, int, int)
PREPARE_AUDIT_GRAPH_UPDATE(ymin_itemid, uint64_t, uint64)
PREPARE_AUDIT_GRAPH_UPDATE(ymax_itemid, uint64_t, uint64)
PREPARE_AUDIT_GRAPH_UPDATE(discover, int, int)
PREPARE_AUDIT_GRAPH_UPDATE(templateid, uint64_t, uint64)

#undef PREPARE_AUDIT_GRAPH_UPDATE
#undef GR_OR_GRP

void	zbx_audit_graph_update_json_update_gitem_create_entry(zbx_uint64_t graphid, int flags, zbx_uint64_t gitemid)
{
	char	audit_key_[AUDIT_DETAILS_KEY_LEN];
	int	resource_type;

	RETURN_IF_AUDIT_OFF();

	resource_type = graph_flag_to_resource_type(flags);

	AUDIT_KEY_GITEMS_SNPRINTF(,)

	zbx_audit_update_json_append_no_value(graphid, AUDIT_DETAILS_ACTION_UPDATE, audit_key_);
}

#define PREPARE_AUDIT_GRAPH_UPDATE(resource, type1, type2)							\
void	zbx_audit_graph_update_json_update_gitem_update_##resource(zbx_uint64_t graphid, int flags,		\
		zbx_uint64_t gitemid, type1 resource##_old, type1 resource##_new)				\
{														\
	char	audit_key_##resource[AUDIT_DETAILS_KEY_LEN];							\
	int	resource_type;											\
														\
	RETURN_IF_AUDIT_OFF();											\
														\
	resource_type = graph_flag_to_resource_type(flags);							\
														\
	AUDIT_KEY_GITEMS_SNPRINTF(resource, .)									\
														\
	zbx_audit_update_json_update_##type2(graphid, audit_key_##resource, resource##_old, resource##_new);	\
}
PREPARE_AUDIT_GRAPH_UPDATE(drawtype,int, int)
PREPARE_AUDIT_GRAPH_UPDATE(sortorder, int, int)
PREPARE_AUDIT_GRAPH_UPDATE(color, const char*, string)
PREPARE_AUDIT_GRAPH_UPDATE(yaxisside, int, int)
PREPARE_AUDIT_GRAPH_UPDATE(calc_fnc, int, int)
PREPARE_AUDIT_GRAPH_UPDATE(type, int, int)

#undef PREPARE_AUDIT_GRAPH_UPDATE
#undef AUDIT_KEY_GITEMS_SNPRINTF

void	zbx_audit_DBselect_delete_for_graph(const char *sql, zbx_vector_uint64_t *ids)
{
	DB_RESULT	result;
	DB_ROW		row;

	result = DBselect("%s", sql);

	while (NULL != (row = DBfetch(result)))
	{
		int		flags;
		zbx_uint64_t	id;

		ZBX_STR2UINT64(id, row[0]);
		zbx_vector_uint64_append(ids, id);
		flags = atoi(row[2]);

		zbx_audit_graph_create_entry(AUDIT_ACTION_DELETE, id, row[1], flags);
	}

	DBfree_result(result);

	zbx_vector_uint64_sort(ids, ZBX_DEFAULT_UINT64_COMPARE_FUNC);
}
