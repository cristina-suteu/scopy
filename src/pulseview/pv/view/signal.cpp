/*
 * This file is part of the PulseView project.
 *
 * Copyright (C) 2012 Joel Holdsworth <joel@airwebreathe.org.uk>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <pulseview/extdef.h>

#include <assert.h>
#include <cmath>

#include <QApplication>
#include <QFormLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>

#include <libsigrokcxx/libsigrokcxx.hpp>

#include "signal.hpp"
#include "view.hpp"

using std::shared_ptr;
using std::make_shared;

using sigrok::Channel;

namespace pv {
namespace view {

const char *const ChannelNames[] = {
	"CLK",
	"DATA",
	"IN",
	"OUT",
	"RST",
	"TX",
	"RX",
	"EN",
	"SCLK",
	"MOSI",
	"MISO",
	"/SS",
	"SDA",
	"SCL"
};

Signal::Signal(pv::Session &session,
	std::shared_ptr<sigrok::Channel> channel) :
	Trace(QString::fromUtf8(channel->name().c_str())),
	session_(session),
	channel_(channel),
	scale_handle_(make_shared<SignalScaleHandle>(*this)),
	items_({scale_handle_}),
	name_widget_(nullptr)
{
	assert(channel_);
}

void Signal::set_name(QString name)
{
	Trace::set_name(name);

	if (name != name_widget_->currentText())
		name_widget_->setEditText(name);

	// Store the channel name in sigrok::Channel so that it
	// will end up in the .sr file upon save.
	channel_->set_name(name.toUtf8().constData());
}

bool Signal::enabled() const
{
	return channel_->enabled();
}

void Signal::enable(bool enable)
{
	channel_->set_enabled(enable);

	if (owner_)
		owner_->extents_changed(true, true);
}

shared_ptr<Channel> Signal::channel() const
{
	return channel_;
}

pv::Session* Signal::session()
{
	return &session_;
}

const ViewItemOwner::item_list& Signal::child_items() const
{
	return items_;
}

void Signal::paint_back(QPainter &p, const ViewItemPaintParams &pp)
{
	if (channel_->enabled())
		Trace::paint_back(p, pp);
}

void Signal::populate_popup_form(QWidget *parent, QFormLayout *form)
{
	name_widget_ = new QComboBox(parent);
	name_widget_->setEditable(true);
	name_widget_->setCompleter(nullptr);

	for (unsigned int i = 0; i < countof(ChannelNames); i++)
		name_widget_->insertItem(i, ChannelNames[i]);

	const int index = name_widget_->findText(name_, Qt::MatchExactly);

	if (index == -1) {
		name_widget_->insertItem(0, name_);
		name_widget_->setCurrentIndex(0);
	} else {
		name_widget_->setCurrentIndex(index);
	}

	connect(name_widget_, SIGNAL(editTextChanged(const QString&)),
		this, SLOT(on_text_changed(const QString&)));

    form->addRow(tr("Name"), name_widget_);

	add_colour_option(parent, form);
}

QMenu* Signal::create_context_menu(QWidget *parent)
{
	QMenu *const menu = Trace::create_context_menu(parent);

	menu->addSeparator();

	QAction *const disable = new QAction(tr("Disable"), this);
	disable->setShortcuts(QKeySequence::Delete);
	connect(disable, SIGNAL(triggered()), this, SLOT(on_disable()));
	menu->addAction(disable);

	return menu;
}

void Signal::delete_pressed()
{
	on_disable();
}

void Signal::on_disable()
{
	enable(false);
}

} // namespace view
} // namespace pv
