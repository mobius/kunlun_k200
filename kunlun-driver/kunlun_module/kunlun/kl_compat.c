/*
 * SPDX-FileCopyrightText: Copyright (c) 2021-2022 KUNLUNXIN CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "kl_compat.h"
#include <linux/version.h>

static struct pci_bus *compat_find_pci_root_bus(struct pci_bus *bus)
{
    while (bus->parent)
        bus = bus->parent;

    return bus;
}

static struct pci_host_bridge *compat_find_pci_host_bridge(struct pci_bus *bus)
{
    struct pci_bus *root_bus = compat_find_pci_root_bus(bus);

    return to_pci_host_bridge(root_bus->bridge);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 0, 0))
#define HAS_PCI_HOST_BRIDGE_WINDOW 0
#elif defined(RHEL_RELEASE_VERSION)
#if (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(7, 3))
#define HAS_PCI_HOST_BRIDGE_WINDOW 0
#else
#define HAS_PCI_HOST_BRIDGE_WINDOW 1
#endif
#else
#define HAS_PCI_HOST_BRIDGE_WINDOW 1
#endif

#if (HAS_PCI_HOST_BRIDGE_WINDOW == 0)

void compat_pcibios_resource_to_bus(struct pci_bus *bus, struct pci_bus_region *region,
                                    struct resource *res)
{
    struct pci_host_bridge *bridge = compat_find_pci_host_bridge(bus);
    struct resource_entry  *window;
    resource_size_t         offset = 0;

    resource_list_for_each_entry(window, &bridge->windows) {
        if (resource_contains(window->res, res)) {
            offset = window->offset;
            break;
        }
    }

    region->start = res->start - offset;
    region->end   = res->end - offset;
}

static bool compat_region_contains(struct pci_bus_region *region1, struct pci_bus_region *region2)
{
    return region1->start <= region2->start && region1->end >= region2->end;
}

void compat_pcibios_bus_to_resource(struct pci_bus *bus, struct resource *res,
                                    struct pci_bus_region *region)
{
    struct pci_host_bridge *bridge = compat_find_pci_host_bridge(bus);
    struct resource_entry  *window;
    resource_size_t         offset = 0;

    resource_list_for_each_entry(window, &bridge->windows) {
        struct pci_bus_region bus_region;

        if (resource_type(res) != resource_type(window->res))
            continue;

        bus_region.start = window->res->start - window->offset;
        bus_region.end   = window->res->end - window->offset;

        if (compat_region_contains(&bus_region, region)) {
            offset = window->offset;
            break;
        }
    }

    res->start = region->start + offset;
    res->end   = region->end + offset;
}

#else

static bool compat_resource_contains(struct resource *res1, struct resource *res2)
{
    return res1->start <= res2->start && res1->end >= res2->end;
}

void compat_pcibios_resource_to_bus(struct pci_bus *bus, struct pci_bus_region *region,
                                    struct resource *res)
{
    struct pci_host_bridge        *bridge = compat_find_pci_host_bridge(bus);
    struct pci_host_bridge_window *window;
    resource_size_t                offset = 0;

    list_for_each_entry(window, &bridge->windows, list) {
        if (resource_type(res) != resource_type(window->res))
            continue;

        if (compat_resource_contains(window->res, res)) {
            offset = window->offset;
            break;
        }
    }

    region->start = res->start - offset;
    region->end   = res->end - offset;
}

static bool compat_region_contains(struct pci_bus_region *region1, struct pci_bus_region *region2)
{
    return region1->start <= region2->start && region1->end >= region2->end;
}

void compat_pcibios_bus_to_resource(struct pci_bus *bus, struct resource *res,
                                    struct pci_bus_region *region)
{
    struct pci_host_bridge        *bridge = compat_find_pci_host_bridge(bus);
    struct pci_host_bridge_window *window;
    resource_size_t                offset = 0;

    list_for_each_entry(window, &bridge->windows, list) {
        struct pci_bus_region bus_region;

        if (resource_type(res) != resource_type(window->res))
            continue;

        bus_region.start = window->res->start - window->offset;
        bus_region.end   = window->res->end - window->offset;

        if (compat_region_contains(&bus_region, region)) {
            offset = window->offset;
            break;
        }
    }

    res->start = region->start + offset;
    res->end   = region->end + offset;
}

#endif
