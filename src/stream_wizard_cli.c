/*****************************************************************************
 * stream_wizard_cli.c: CLI stream wizard configuration parser
 *****************************************************************************
 * Copyright (C) 2025 the VideoLAN team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_playlist.h>
#include "libvlc.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* Include the C++ wrapper header */
#include "../modules/gui/qt/cli_stream_wrapper.hpp"

/**
 * Helper function to trim whitespace from a string
 */
static char *trim_whitespace(char *str)
{
    char *end;

    /* Trim leading space */
    while (isspace((unsigned char)*str)) str++;

    if (*str == 0)
        return str;

    /* Trim trailing space */
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    /* Write new null terminator */
    *(end + 1) = 0;

    return str;
}

/**
 * Helper function to remove quotes from a string
 */
static char *remove_quotes(char *str)
{
    size_t len = strlen(str);
    if (len >= 2 && ((str[0] == '\'' && str[len-1] == '\'') ||
                     (str[0] == '"' && str[len-1] == '"')))
    {
        str[len-1] = '\0';
        return str + 1;
    }
    return str;
}

/**
 * Parse the stream-wizard-config string and apply it to the libvlc instance
 *
 * Expected format:
 * profile=<name>,dest=<type>,addr=<address>,port=<port>,path=<path>,
 * transcode=<0|1>,local=<0|1>,sout_all=<0|1>
 */
char *vlc_ParseStreamWizardConfig(libvlc_int_t *libvlc, const char *config_str)
{
    if (!config_str || !*config_str)
        return NULL;

    CLIStreamParams params;
    memset(&params, 0, sizeof(params));

    /* Set defaults */
    params.transcode_enable = 0;
    params.port = -1;
    params.sout_all = 1;
    params.local_output = 0;

    /* Make a copy of the config string for parsing */
    char *config = strdup(config_str);
    if (!config)
        return NULL;

    /* Parse key=value pairs */
    char *saveptr;
    char *token = strtok_r(config, ",", &saveptr);

    while (token)
    {
        char *key = trim_whitespace(token);
        char *eq = strchr(key, '=');

        if (eq)
        {
            *eq = '\0';
            char *value = trim_whitespace(eq + 1);
            value = remove_quotes(value);
            key = trim_whitespace(key);

            if (strcmp(key, "profile") == 0)
                params.profile_name = strdup(value);
            else if (strcmp(key, "dest") == 0)
                params.destination_type = strdup(value);
            else if (strcmp(key, "addr") == 0 || strcmp(key, "address") == 0)
                params.address = strdup(value);
            else if (strcmp(key, "port") == 0)
                params.port = atoi(value);
            else if (strcmp(key, "path") == 0)
                params.path = strdup(value);
            else if (strcmp(key, "mux") == 0)
                params.mux = strdup(value);
            else if (strcmp(key, "transcode") == 0)
                params.transcode_enable = atoi(value);
            else if (strcmp(key, "local") == 0)
                params.local_output = atoi(value);
            else if (strcmp(key, "sout_all") == 0)
                params.sout_all = atoi(value);
            else if (strcmp(key, "sap_name") == 0)
                params.sap_name = strdup(value);
            else if (strcmp(key, "ice_mount") == 0)
                params.ice_mount = strdup(value);
            else if (strcmp(key, "ice_password") == 0 || strcmp(key, "ice_pass") == 0)
                params.ice_password = strdup(value);
            else
                msg_Warn(libvlc, "Unknown stream wizard parameter: %s", key);
        }

        token = strtok_r(NULL, ",", &saveptr);
    }

    free(config);

    /* Validate required parameters */
    if (!params.destination_type || !*params.destination_type)
    {
        msg_Err(libvlc, "Stream wizard: destination type is required");
        goto error;
    }

    /* Call the C++ wrapper to generate the sout chain */
    char *sout_chain = vlc_GenerateSoutStringFromCLI(&params);

    /* Cleanup allocated strings */
error:
    if (params.profile_name) free((void*)params.profile_name);
    if (params.destination_type) free((void*)params.destination_type);
    if (params.address) free((void*)params.address);
    if (params.path) free((void*)params.path);
    if (params.mux) free((void*)params.mux);
    if (params.sap_name) free((void*)params.sap_name);
    if (params.ice_mount) free((void*)params.ice_mount);
    if (params.ice_password) free((void*)params.ice_password);

    return sout_chain;
}

/**
 * Apply stream wizard configuration from command line
 * This function should be called during libvlc initialization
 */
void vlc_ApplyStreamWizardConfig(libvlc_int_t *libvlc)
{
    char *config = var_InheritString(libvlc, "stream-wizard-config");
    if (!config || !*config)
    {
        free(config);
        return;
    }

    msg_Dbg(libvlc, "Parsing stream wizard configuration: %s", config);

    char *sout_chain = vlc_ParseStreamWizardConfig(libvlc, config);
    free(config);

    if (sout_chain)
    {
        msg_Info(libvlc, "Generated stream output chain: %s", sout_chain);

        /* Set the sout variable */
        var_SetString(libvlc, "sout", sout_chain);

        vlc_FreeSoutString(sout_chain);
    }
    else
    {
        msg_Err(libvlc, "Failed to generate stream output chain from wizard config");
    }
}
