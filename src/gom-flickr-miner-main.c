/*
 * GNOME Online Miners - crawls through your online content
 * Copyright (c) 2013 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Author: Marek Chalupa <mchalupa@redhat.com>
 */

#include "config.h"

#define INSIDE_MINER
#define MINER_NAME "FLICKR"
#define MINER_TYPE GOM_TYPE_FLICKR_MINER
#define MINER_BUS_NAME "org.gnome.OnlineMiners.Flickr"
#define MINER_OBJECT_PATH "/org/gnome/OnlineMiners/Flickr"

#include "gom-flickr-miner.h"
#include "gom-miner-main.c"
