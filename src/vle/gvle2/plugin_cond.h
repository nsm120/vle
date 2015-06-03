/*
 * This file is part of VLE, a framework for multi-modeling, simulation
 * and analysis of complex dynamical systems.
 * http://www.vle-project.org
 *
 * Copyright (c) 2014-2015 INRA http://www.inra.fr
 *
 * See the AUTHORS or Authors.txt file for copyright owners and
 * contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PLUGIN_COND_H
#define PLUGIN_COND_H

#include <QObject>
#include <QString>
#include <QWidget>
#include <vle/gvle2/logger.h>
#include <vle/gvle2/vlevpz.h>

class PluginExpCond: public QObject
{
    Q_OBJECT
public:
    virtual QString  getname()   = 0;
    virtual QWidget *getWidget() = 0;
    virtual void     delWidget() = 0;
    virtual QWidget *getWidgetToolbar() = 0;
    virtual void     delWidgetToolbar() = 0;
    virtual void  setSettings(QSettings *s) = 0;
    virtual void  setLogger(Logger *logger) = 0;
    virtual void  setExpCond(const QString& condName) = 0;
    virtual void  setVpz(vleVpz* vpz) = 0;
    virtual void *getVpz() = 0;
//signals:
//    virtual void  valueChanged() = 0;
};

Q_DECLARE_INTERFACE(PluginExpCond, "fr.inra.vle.gvle2.PluginExpCond/1.0")

#endif // PLUGIN_COND_H