/* Buzztrax
 * Copyright (C) 2017 Buzztrax team <buzztrax-devel@buzztrax.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#define BT_CORE
#define BT_EXPERIMENTS_C

#include "core_private.h"

static BtExperimentFlags active_experiments;

/**
 * bt_experiments_init:
 * @flags: a string array of experiments to activate
 *
 * Parses the experiemnts. The coe can later use bt_experiments_check_active()
 * to check the flags.
 */
void
bt_experiments_init (gchar ** flags)
{
  gint i;
  gchar *flag;

  active_experiments = 0;
  for (i = 0; flags[i]; i++) {
    flag = g_ascii_strdown (flags[i], -1);
    GST_INFO ("  experiment: '%s'", flag);

    // When updating these, also update core.c:bt_init_get_option_group()
    if (!strcmp (flag, "audiomixer")) {
      active_experiments |= BT_EXPERIMENT_AUDIO_MIXER;
    } else {
      GST_WARNING ("unknown experiment: '%s'", flags[i]);
    }
    /* TODO: add more experiments:
     * machine.c: TEE_MERGE_EVENT_HACK
     */
  }
}

/**
 * bt_experiments_check_active:
 * @experiments: one or more experiments to check for
 *
 * Check if the experimental code should run.
 *
 * Returns: %TRUE if all experiments are active
 */
gboolean
bt_experiments_check_active (BtExperimentFlags experiments)
{
  return (active_experiments & experiments) == experiments;
}
