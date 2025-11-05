/*****************************************************************************
 * stream_wizard_cli.h: CLI stream wizard configuration parser
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

#ifndef VLC_STREAM_WIZARD_CLI_H
#define VLC_STREAM_WIZARD_CLI_H

/**
 * Apply stream wizard configuration from command line
 *
 * This function parses the --stream-wizard-config option and generates
 * a stream output chain using the C++ wrapper functions from the Qt GUI.
 *
 * @param libvlc The libvlc instance
 */
void vlc_ApplyStreamWizardConfig(libvlc_int_t *libvlc);

/**
 * Parse stream wizard configuration string
 *
 * @param libvlc The libvlc instance
 * @param config_str The configuration string to parse
 * @return Newly allocated sout chain string (caller must free), or NULL on error
 */
char *vlc_ParseStreamWizardConfig(libvlc_int_t *libvlc, const char *config_str);

#endif /* VLC_STREAM_WIZARD_CLI_H */
