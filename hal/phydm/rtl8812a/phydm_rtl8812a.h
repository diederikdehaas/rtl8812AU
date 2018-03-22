/******************************************************************************
 *
 * Copyright(c) 2007 - 2017 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/

#ifndef	__ODM_RTL8812A_H__
#define __ODM_RTL8812A_H__
#if (defined(CONFIG_PATH_DIVERSITY))

void
odm_path_statistics_8812a(
	struct PHY_DM_STRUCT		*p_dm,
	u32			mac_id,
	u32			RSSI_A,
	u32			RSSI_B
);

void
odm_path_diversity_init_8812a(struct PHY_DM_STRUCT	*p_dm);

void
odm_path_diversity_8812a(struct PHY_DM_STRUCT	*p_dm);

void
odm_set_tx_path_by_tx_info_8812a(
	struct PHY_DM_STRUCT		*p_dm,
	u8			*p_desc,
	u8			mac_id
);
#endif
#endif
