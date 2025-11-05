/*****************************************************************************
 * cli_stream_wrapper.cpp: C wrapper for stream output chain generation
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

#include "cli_stream_wrapper.hpp"
#include "dialogs/sout/profiles.hpp"
#include "util/soutchain.hpp"

#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QUrl>
#include <QHash>
#include <cstring>
#include <cstdlib>

/**
 * Helper function to build transcode chain from profile string
 * This replicates the logic from VLCProfileSelector::updateOptions()
 */
static SoutChain buildTranscodeChain(const QString& profileValue, QString& outMux)
{
    SoutChain transcode;

    if (profileValue.isEmpty() || !profileValue.contains(";"))
        return transcode;

    transcode.clear();
    QStringList tuples = profileValue.split(";");

    typedef QHash<QString, QString> proptovalueHashType;
    QHash<QString, proptovalueHashType *> categtopropHash;
    proptovalueHashType *proptovalueHash;
    QString value;

    /* Build hash structure from profile string */
    foreach (const QString &tuple, tuples)
    {
        QStringList keyvalue = tuple.split("=");
        if (keyvalue.count() != 2) continue;
        QString key = keyvalue[0];
        value = keyvalue[1];
        keyvalue = key.split("_");
        if (keyvalue.count() != 2) continue;
        QString categ = keyvalue[0];
        QString prop = keyvalue[1];

        if (!categtopropHash.contains(categ))
        {
            proptovalueHash = new proptovalueHashType();
            categtopropHash.insert(categ, proptovalueHash);
        }
        else
        {
            proptovalueHash = categtopropHash.value(categ);
        }
        proptovalueHash->insert(prop, value);
    }

#define HASHPICK( categ, prop ) \
    if ( categtopropHash.contains( categ ) ) \
    {\
        proptovalueHash = categtopropHash.value( categ );\
        value = proptovalueHash->take( prop );\
    }\
    else value = QString()

    transcode.begin("transcode");

    /* Extract muxer */
    HASHPICK("muxer", "mux");
    if (value.isEmpty())
    {
        /* Cleanup and return empty chain */
        qDeleteAll(categtopropHash);
        return SoutChain();
    }
    outMux = value;

    /* Video codec */
    HASHPICK("video", "enable");
    if (!value.isEmpty())
    {
        HASHPICK("video", "codec");
        if (!value.isEmpty())
        {
            transcode.option("vcodec", value);

            HASHPICK("vcodec", "bitrate");
            if (value.toInt() > 0)
                transcode.option("vb", value.toInt());

            HASHPICK("video", "filters");
            if (!value.isEmpty())
            {
                QStringList valuesList = QUrl::fromPercentEncoding(value.toLatin1()).split(";");
                transcode.option("vfilter", valuesList.join(":"));
            }

            /* H264 special options */
            QStringList codecoptions;
            HASHPICK("vcodec", "qp");
            if (value.toInt() > 0)
                codecoptions << QString("qp=%1").arg(value);

            HASHPICK("vcodec", "custom");
            if (!value.isEmpty())
                codecoptions << QUrl::fromPercentEncoding(value.toLatin1());

            if (codecoptions.count())
                transcode.option("venc", QString("x264{%1}").arg(codecoptions.join(",")));

            HASHPICK("vcodec", "framerate");
            if (!value.isEmpty() && value.toInt() > 0)
                transcode.option("fps", value);

            HASHPICK("vcodec", "scale");
            if (!value.isEmpty())
                transcode.option("scale", value);

            HASHPICK("vcodec", "width");
            if (!value.isEmpty() && value.toInt() > 0)
                transcode.option("width", value);

            HASHPICK("vcodec", "height");
            if (!value.isEmpty() && value.toInt() > 0)
                transcode.option("height", value);
        }
    }
    else
    {
        transcode.option("vcodec", "none");
    }

    /* Audio codec */
    HASHPICK("audio", "enable");
    if (!value.isEmpty())
    {
        HASHPICK("audio", "codec");
        if (!value.isEmpty())
        {
            transcode.option("acodec", value);

            HASHPICK("acodec", "bitrate");
            transcode.option("ab", value.toInt());

            HASHPICK("acodec", "channels");
            transcode.option("channels", value.toInt());

            HASHPICK("acodec", "samplerate");
            transcode.option("samplerate", value.toInt());

            HASHPICK("audio", "filters");
            if (!value.isEmpty())
            {
                QStringList valuesList = QUrl::fromPercentEncoding(value.toLatin1()).split(";");
                transcode.option("afilter", valuesList.join(":"));
            }
        }
    }
    else
    {
        transcode.option("acodec", "none");
    }

    /* Subtitles */
    HASHPICK("subtitles", "enable");
    if (!value.isEmpty())
    {
        HASHPICK("subtitles", "overlay");
        if (value.isEmpty())
        {
            HASHPICK("subtitles", "codec");
            if (!value.isEmpty())
                transcode.option("scodec", value);
        }
        else
        {
            transcode.option("soverlay");
        }
    }
    else
    {
        transcode.option("scodec", "none");
    }

    transcode.end();

#undef HASHPICK

    /* Cleanup */
    qDeleteAll(categtopropHash);

    return transcode;
}

/**
 * Helper function to find profile value by name
 */
static QString findProfileValue(const char *profileName)
{
    if (!profileName || !*profileName)
        return QString();

    QString name(profileName);

    /* Search in predefined profiles */
    for (size_t i = 0; i < NB_PROFILE; i++)
    {
        if (name == video_profile_name_list[i])
            return QString(video_profile_value_list[i]);
    }

    return QString();
}

/**
 * Helper function to build destination MRL
 * Replicates logic from various *DestBox::getMRL() methods
 */
static QString buildDestinationMRL(const CLIStreamParams *params, const QString& mux)
{
    SoutChain m;
    QString destType(params->destination_type);

    if (destType == "file")
    {
        if (!params->address || !*params->address)
            return QString();

        m.begin("file");
        QString outputfile(params->address);

        if (!mux.isEmpty())
        {
            if (outputfile.contains(QRegularExpression(QStringLiteral("\\..{2,4}$"))) &&
                !outputfile.endsWith(mux))
            {
                outputfile.replace(QRegularExpression(QStringLiteral("\\..{2,4}$")), "." + mux);
            }
            else if (!outputfile.endsWith(mux))
            {
                m.option("mux", mux);
            }
        }
        m.option("dst", outputfile);
        m.option("no-overwrite");
        m.end();
    }
    else if (destType == "http")
    {
        QString path = params->path ? QString(params->path) : QString("/");
        if (path[0] != '/')
            path.prepend("/");

        QString port = QString::number(params->port > 0 ? params->port : 8080);
        QString dst = ":" + port + path;

        m.begin("http");
        if (!path.contains(QRegularExpression(QStringLiteral("\\..{2,3}$"))))
        {
            if (!mux.isEmpty() && mux != "mp4")
                m.option("mux", mux);
            else
                m.option("mux", "ffmpeg{mux=flv}");
        }
        m.option("dst", dst);
        m.end();
    }
    else if (destType == "rtsp")
    {
        QString path = params->path ? QString(params->path) : QString("/");
        if (path[0] != '/')
            path.prepend("/");

        QString port = QString::number(params->port > 0 ? params->port : 8554);
        QString sdp = "rtsp://:" + port + path;

        m.begin("rtp");
        m.option("sdp", sdp);
        m.end();
    }
    else if (destType == "mmsh")
    {
        if (!params->address || !*params->address)
            return QString();

        m.begin("std");
        m.option("access", "mmsh");
        m.option("mux", "asfh");
        m.option("dst", QString(params->address), params->port > 0 ? params->port : 8080);
        m.end();
    }
    else if (destType == "udp")
    {
        if (!params->address || !*params->address)
            return QString();

        m.begin("udp");
        if (!mux.isEmpty() && mux == "ts")
            m.option("mux", mux);
        m.option("dst", QString(params->address), params->port > 0 ? params->port : 1234);
        m.end();
    }
    else if (destType == "srt")
    {
        if (!params->address || !*params->address)
            return QString();

        QString destination = QString(params->address) + ":" +
                            QString::number(params->port > 0 ? params->port : 7001);

        m.begin("srt");
        m.option("dst", destination);
        if (!mux.isEmpty())
            m.option("mux", mux);
        if (params->sap_name && *params->sap_name)
        {
            m.option("sap");
            m.option("name", QString(params->sap_name));
        }
        m.end();
    }
    else if (destType == "rist")
    {
        if (!params->address || !*params->address)
            return QString();

        QString destination = QString(params->address) + ":" +
                            QString::number(params->port > 0 ? params->port : 1968);

        m.begin("std");
        if (params->sap_name && *params->sap_name)
            m.option("access", "rist{stream-name=" + QString(params->sap_name) + "}");
        else
            m.option("access", "rist");
        m.option("mux", "ts");
        m.option("dst", destination);
        m.end();
    }
    else if (destType == "rtp")
    {
        if (!params->address || !*params->address)
            return QString();

        m.begin("rtp");
        m.option("dst", QString(params->address));
        m.option("port", params->port > 0 ? params->port : 5004);
        if (!mux.isEmpty())
            m.option("mux", mux);
        if (params->sap_name && *params->sap_name)
        {
            m.option("sap");
            m.option("name", QString(params->sap_name));
        }
        m.end();
    }
    else if (destType == "ice" || destType == "icecast")
    {
        if (!params->address || !*params->address)
            return QString();

        m.begin("std");
        m.option("access", "shout");
        m.option("mux", "ogg");

        QString password = params->ice_password ? QString(params->ice_password) : QString();
        QString mount = params->ice_mount ? QString(params->ice_mount) : QString();
        QString url = "//" + password + "@" + QString(params->address) + ":" +
                     QString::number(params->port > 0 ? params->port : 8000) + "/" + mount;

        m.option("dst", url);
        m.end();
    }
    else
    {
        return QString();
    }

    return m.to_string();
}

/**
 * Main C-callable function to generate sout chain
 */
extern "C" char *vlc_GenerateSoutStringFromCLI(const CLIStreamParams *params)
{
    if (!params)
        return NULL;

    QString chain;
    SoutChain smrl(":sout=#");
    QString mux;

    /* Build transcode chain if enabled */
    if (params->transcode_enable && params->profile_name && *params->profile_name)
    {
        QString profileValue = findProfileValue(params->profile_name);
        if (!profileValue.isEmpty())
        {
            SoutChain transcodeChain = buildTranscodeChain(profileValue, mux);
            if (!transcodeChain.to_string().isEmpty())
            {
                smrl.begin(transcodeChain.to_string());
                smrl.end();
            }
        }
    }

    /* Override mux if specified */
    if (params->mux && *params->mux)
        mux = QString(params->mux);

    /* Build destination MRL */
    QString destMRL = buildDestinationMRL(params, mux);
    if (destMRL.isEmpty())
        return NULL;

    /* Add destination to chain */
    bool multi = params->local_output;

    if (multi)
        smrl.begin("duplicate");

    if (multi)
        smrl.option("dst", destMRL, true);
    else
    {
        smrl.begin(destMRL);
        smrl.end();
    }

    /* Add local display if requested */
    if (params->local_output)
    {
        smrl.option("dst", "display");
        smrl.end(); // End duplicate
    }

    chain = smrl.to_string();

    /* Add sout-all option */
    if (params->sout_all)
        chain.append(" :sout-all");
    else
        chain.append(" :no-sout-all");

    chain.append(" :sout-keep");

    /* Convert to C string */
    QByteArray ba = chain.toUtf8();
    return strdup(ba.constData());
}

/**
 * Free function for sout string
 */
extern "C" void vlc_FreeSoutString(char *str)
{
    free(str);
}
