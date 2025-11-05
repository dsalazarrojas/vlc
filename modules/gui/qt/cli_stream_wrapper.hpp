/*****************************************************************************
 * cli_stream_wrapper.hpp: C wrapper for stream output chain generation
 *****************************************************************************
 * Copyright (C) 2025 the VideoLAN team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef VLC_CLI_STREAM_WRAPPER_HPP_
#define VLC_CLI_STREAM_WRAPPER_HPP_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Parameters for CLI-based stream output generation
 * This structure mirrors the options available in the Qt stream dialog
 */
typedef struct CLIStreamParams {
    /* Profile settings */
    const char *profile_name;           /* Profile name from profiles.hpp */
    int transcode_enable;               /* Enable transcoding (0 or 1) */

    /* Destination settings */
    const char *destination_type;       /* "file", "http", "rtsp", "udp", "rtp", "srt", "rist", "mmsh", "ice" */

    /* Common destination parameters */
    const char *address;                /* IP address or file path */
    int port;                           /* Port number (for network destinations) */
    const char *path;                   /* Path (for HTTP/RTSP) or mount point */

    /* Additional options */
    const char *mux;                    /* Muxer override (optional, can be NULL) */
    int sout_all;                       /* Enable sout-all (0 or 1) */
    int local_output;                   /* Display locally while streaming (0 or 1) */

    /* Advanced parameters (optional) */
    const char *sap_name;               /* SAP announcement name (for RTP/SRT) */
    const char *ice_mount;              /* Icecast mount point */
    const char *ice_password;           /* Icecast password */
} CLIStreamParams;

/**
 * Generate a stream output chain string from CLI parameters
 *
 * This function replicates the logic from SoutDialog::updateChain()
 * to generate a sout chain string that can be used as a media option.
 *
 * @param params Pointer to CLIStreamParams structure with stream configuration
 * @return Newly allocated string containing the sout chain (caller must free()),
 *         or NULL on error
 */
char *vlc_GenerateSoutStringFromCLI(const CLIStreamParams *params);

/**
 * Free a string returned by vlc_GenerateSoutStringFromCLI
 *
 * @param str String to free
 */
void vlc_FreeSoutString(char *str);

#ifdef __cplusplus
}
#endif

#endif // VLC_CLI_STREAM_WRAPPER_HPP_
