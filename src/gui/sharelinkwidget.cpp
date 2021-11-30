/*
 * Copyright (C) by Roeland Jago Douma <roeland@famdouma.nl>
 * Copyright (C) 2015 by Klaas Freitag <freitag@owncloud.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 */

#include "ui_sharelinkwidget.h"
#include "sharelinkwidget.h"
#include "account.h"
#include "capabilities.h"
#include "guiutility.h"
#include "sharemanager.h"
#include "theme.h"
#include "elidedlabel.h"

#include "QProgressIndicator.h"
#include <QBuffer>
#include <QClipboard>
#include <QFileInfo>
#include <QDesktopServices>
#include <QMessageBox>
#include <QMenu>
#include <QTextEdit>
#include <QToolButton>
#include <QPropertyAnimation>

namespace {
    const char *passwordIsSetPlaceholder = "●●●●●●●●";
}

namespace OCC {

Q_LOGGING_CATEGORY(lcShareLink, "nextcloud.gui.sharelink", QtInfoMsg)

ShareLinkWidget::ShareLinkWidget(AccountPtr account,
    const QString &sharePath,
    const QString &localPath,
    SharePermissions maxSharingPermissions,
    QWidget *parent)
    : QWidget(parent)
    , _ui(new Ui::ShareLinkWidget)
    , _account(account)
    , _sharePath(sharePath)
    , _localPath(localPath)
    , _linkShare(nullptr)
    , _passwordRequired(false)
    , _expiryRequired(false)
    , _namesSupported(true)
    , _noteRequired(false)
    , _linkContextMenu(nullptr)
    , _readOnlyLinkAction(nullptr)
    , _allowEditingLinkAction(nullptr)
    , _allowUploadEditingLinkAction(nullptr)
    , _allowUploadLinkAction(nullptr)
    , _passwordProtectLinkAction(nullptr)
    , _expirationDateLinkAction(nullptr)
    , _unshareLinkAction(nullptr)
    , _noteLinkAction(nullptr)
{
    _ui->setupUi(this);

    _ui->shareLinkToolButton->hide();

    //Is this a file or folder?
    QFileInfo fi(localPath);
    _isFile = fi.isFile();

    connect(_ui->enableShareLink, &QPushButton::clicked, this, &ShareLinkWidget::slotCreateShareLink);
    connect(_ui->lineEdit_password, &QLineEdit::returnPressed, this, &ShareLinkWidget::slotCreatePassword);
    connect(_ui->confirmPassword, &QAbstractButton::clicked, this, &ShareLinkWidget::slotCreatePassword);
    connect(_ui->confirmNote, &QAbstractButton::clicked, this, &ShareLinkWidget::slotCreateNote);
    connect(_ui->confirmExpirationDate, &QAbstractButton::clicked, this, &ShareLinkWidget::slotSetExpireDate);

    _ui->errorLabel->hide();

    auto sharingPossible = true;
    if (!_account->capabilities().sharePublicLink()) {
        qCWarning(lcShareLink) << "Link shares have been disabled";
        sharingPossible = false;
    } else if (!(maxSharingPermissions & SharePermissionShare)) {
        qCWarning(lcShareLink) << "The file can not be shared because it was shared without sharing permission.";
        sharingPossible = false;
    }

    _ui->enableShareLink->setChecked(false);
    _ui->shareLinkToolButton->setEnabled(false);
    _ui->shareLinkToolButton->hide();

    // Older servers don't support multiple public link shares
    if (!_account->capabilities().sharePublicLinkMultiple()) {
        _namesSupported = false;
    }

    togglePasswordOptions(false);
    toggleExpireDateOptions(false);
    toggleNoteOptions(false);

    _ui->noteProgressIndicator->setVisible(false);
    _ui->passwordProgressIndicator->setVisible(false);
    _ui->expirationDateProgressIndicator->setVisible(false);
    _ui->sharelinkProgressIndicator->setVisible(false);

    // check if the file is already inside of a synced folder
    if (sharePath.isEmpty()) {
        qCWarning(lcShareLink) << "Unable to share files not in a sync folder.";
        return;
    }
}

ShareLinkWidget::~ShareLinkWidget()
{
    delete _ui;
}

void ShareLinkWidget::slotToggleShareLinkAnimation(const bool start)
{
    _ui->sharelinkProgressIndicator->setVisible(start);
    if (start) {
        if (!_ui->sharelinkProgressIndicator->isAnimated()) {
            _ui->sharelinkProgressIndicator->startAnimation();
        }
    } else {
        _ui->sharelinkProgressIndicator->stopAnimation();
    }
}

void ShareLinkWidget::toggleButtonAnimation(QToolButton *button, QProgressIndicator *progressIndicator, const QAction *checkedAction, const bool start)
{
    if (!progressIndicator->isAnimated() && checkedAction->isChecked()) {
        progressIndicator->startAnimation();
    } else {
        progressIndicator->stopAnimation();
    }

    const auto startAnimation = checkedAction->isChecked()? progressIndicator->isAnimated() : checkedAction->isChecked();
    button->setVisible(!startAnimation && checkedAction->isChecked());
    progressIndicator->setVisible(startAnimation && checkedAction->isChecked());
}

void ShareLinkWidget::setLinkShare(QSharedPointer<LinkShare> linkShare)
{
    _linkShare = linkShare;
}

QSharedPointer<LinkShare> ShareLinkWidget::getLinkShare()
{
    return _linkShare;
}

void ShareLinkWidget::focusPasswordLineEdit()
{
    _ui->lineEdit_password->setFocus();
}

void ShareLinkWidget::setupUiOptions()
{
    connect(_linkShare.data(), &LinkShare::noteSet, this, &ShareLinkWidget::slotNoteSet);
    connect(_linkShare.data(), &LinkShare::passwordSet, this, &ShareLinkWidget::slotPasswordSet);
    connect(_linkShare.data(), &LinkShare::passwordSetError, this, &ShareLinkWidget::slotPasswordSetError);
    connect(_linkShare.data(), &LinkShare::labelSet, this, &ShareLinkWidget::slotLabelSet);

    // Prepare permissions check and create group action
    const QDate expireDate = _linkShare.data()->getExpireDate().isValid() ? _linkShare.data()->getExpireDate() : QDate();
    const SharePermissions perm = _linkShare.data()->getPermissions();
    auto checked = false;
    auto *permissionsGroup = new QActionGroup(this);

    // Prepare sharing menu
    _linkContextMenu = new QMenu(this);

    // radio button style
    permissionsGroup->setExclusive(true);

    if (_isFile) {
        checked = (perm & SharePermissionRead) && (perm & SharePermissionUpdate);
        _allowEditingLinkAction = _linkContextMenu->addAction(tr("Allow editing"));
        _allowEditingLinkAction->setCheckable(true);
        _allowEditingLinkAction->setChecked(checked);

    } else {
        checked = (perm == SharePermissionRead);
        _readOnlyLinkAction = permissionsGroup->addAction(tr("View only"));
        _readOnlyLinkAction->setCheckable(true);
        _readOnlyLinkAction->setChecked(checked);

        checked = (perm & SharePermissionRead) && (perm & SharePermissionCreate)
            && (perm & SharePermissionUpdate) && (perm & SharePermissionDelete);
        _allowUploadEditingLinkAction = permissionsGroup->addAction(tr("Allow upload and editing"));
        _allowUploadEditingLinkAction->setCheckable(true);
        _allowUploadEditingLinkAction->setChecked(checked);

        checked = (perm == SharePermissionCreate);
        _allowUploadLinkAction = permissionsGroup->addAction(tr("File drop (upload only)"));
        _allowUploadLinkAction->setCheckable(true);
        _allowUploadLinkAction->setChecked(checked);
    }
    
    _shareLinkElidedLabel = new OCC::ElidedLabel(this);
    _shareLinkElidedLabel->setElideMode(Qt::ElideRight);
    displayShareLinkLabel();
    _ui->horizontalLayout->insertWidget(2, _shareLinkElidedLabel);
    
    _shareLinkLayout = new QHBoxLayout(this);
    
    _shareLinkLabel = new QLabel(this);
    _shareLinkLabel->setPixmap(QString(":/client/theme/black/edit.svg"));
    _shareLinkLayout->addWidget(_shareLinkLabel);
    
    _shareLinkEdit = new QLineEdit(this);
    connect(_shareLinkEdit, &QLineEdit::returnPressed, this, &ShareLinkWidget::slotCreateLabel);
    _shareLinkEdit->setPlaceholderText(tr("Link name"));
    _shareLinkEdit->setText(_linkShare.data()->getLabel());
    _shareLinkLayout->addWidget(_shareLinkEdit);
    
    _shareLinkButton = new QToolButton(this);
    connect(_shareLinkButton, &QToolButton::clicked, this, &ShareLinkWidget::slotCreateLabel);
    _shareLinkButton->setIcon(QIcon(":/client/theme/confirm.svg"));
    _shareLinkButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    _shareLinkLayout->addWidget(_shareLinkButton);
    
    _shareLinkProgressIndicator = new QProgressIndicator(this);
    _shareLinkProgressIndicator->setVisible(false);
    _shareLinkLayout->addWidget(_shareLinkProgressIndicator);
        
    _shareLinkDefaultWidget = new QWidget(this);
    _shareLinkDefaultWidget->setLayout(_shareLinkLayout);
    
    _shareLinkWidgetAction = new QWidgetAction(this);
    _shareLinkWidgetAction->setDefaultWidget(_shareLinkDefaultWidget);
    _shareLinkWidgetAction->setCheckable(true);
    _linkContextMenu->addAction(_shareLinkWidgetAction);

    // Adds permissions actions (radio button style)
    if (_isFile) {
        _linkContextMenu->addAction(_allowEditingLinkAction);
    } else {
        _linkContextMenu->addAction(_readOnlyLinkAction);
        _linkContextMenu->addAction(_allowUploadEditingLinkAction);
        _linkContextMenu->addAction(_allowUploadLinkAction);
    }

    // Adds action to display note widget (check box)
    _noteLinkAction = _linkContextMenu->addAction(tr("Note to recipient"));
    _noteLinkAction->setCheckable(true);

    if (_linkShare->getNote().isSimpleText() && !_linkShare->getNote().isEmpty()) {
        _ui->textEdit_note->setText(_linkShare->getNote());
        _noteLinkAction->setChecked(true);
        toggleNoteOptions(true);
    }

    // Adds action to display password widget (check box)
    _passwordProtectLinkAction = _linkContextMenu->addAction(tr("Password protect"));
    _passwordProtectLinkAction->setCheckable(true);

    if (_linkShare.data()->isPasswordSet()) {
        _passwordProtectLinkAction->setChecked(true);
        _ui->lineEdit_password->setPlaceholderText(QString::fromUtf8(passwordIsSetPlaceholder));
        togglePasswordOptions(true);
    }

    // If password is enforced then don't allow users to disable it
    if (_account->capabilities().sharePublicLinkEnforcePassword()) {
        if (_linkShare.data()->isPasswordSet()) {
            _passwordProtectLinkAction->setChecked(true);
            _passwordProtectLinkAction->setEnabled(false);
        }
        _passwordRequired = true;
    }

    // Adds action to display expiration date widget (check box)
    _expirationDateLinkAction = _linkContextMenu->addAction(tr("Set expiration date"));
    _expirationDateLinkAction->setCheckable(true);
    if (!expireDate.isNull()) {
        _ui->calendar->setDate(expireDate);
        _expirationDateLinkAction->setChecked(true);
        toggleExpireDateOptions(true);
    }
    connect(_ui->calendar, &QDateTimeEdit::dateChanged, this, &ShareLinkWidget::slotSetExpireDate);
    connect(_linkShare.data(), &LinkShare::expireDateSet, this, &ShareLinkWidget::slotExpireDateSet);
    

    // If expiredate is enforced do not allow disable and set max days
    if (_account->capabilities().sharePublicLinkEnforceExpireDate()) {
        _ui->calendar->setMaximumDate(QDate::currentDate().addDays(
            _account->capabilities().sharePublicLinkExpireDateDays()));
        _expirationDateLinkAction->setChecked(true);
        _expirationDateLinkAction->setEnabled(false);
        _expiryRequired = true;
    }

    // Adds action to unshare widget (check box)
    _unshareLinkAction = _linkContextMenu->addAction(QIcon(":/client/theme/delete.svg"),
        tr("Delete link"));

    _linkContextMenu->addSeparator();

    _addAnotherLinkAction = _linkContextMenu->addAction(QIcon(":/client/theme/add.svg"),
        tr("Add another link"));

    _ui->enableShareLink->setIcon(QIcon(":/client/theme/copy.svg"));
    disconnect(_ui->enableShareLink, &QPushButton::clicked, this, &ShareLinkWidget::slotCreateShareLink);
    connect(_ui->enableShareLink, &QPushButton::clicked, this, &ShareLinkWidget::slotCopyLinkShare);

    connect(_linkContextMenu, &QMenu::triggered,
        this, &ShareLinkWidget::slotLinkContextMenuActionTriggered);

    _ui->shareLinkToolButton->setMenu(_linkContextMenu);
    _ui->shareLinkToolButton->setEnabled(true);
    _ui->enableShareLink->setEnabled(true);
    _ui->enableShareLink->setChecked(true);

    // show sharing options
    _ui->shareLinkToolButton->show();

    customizeStyle();
}

void ShareLinkWidget::slotCreateNote()                                                                              
{
    const auto note = _ui->textEdit_note->toPlainText();
    if (!_linkShare || _linkShare->getNote() == note || note.isEmpty()) {
        return;
    }
    
    toggleButtonAnimation(_ui->confirmNote, _ui->noteProgressIndicator, _noteLinkAction, true);
    _ui->errorLabel->hide();
    _linkShare->setNote(note);
}

void ShareLinkWidget::slotNoteSet()
{
    toggleButtonAnimation(_ui->confirmNote, _ui->noteProgressIndicator, _noteLinkAction, false);
}

void ShareLinkWidget::slotCopyLinkShare(const bool clicked)
{
    Q_UNUSED(clicked);

    QApplication::clipboard()->setText(_linkShare->getLink().toString());
}

void ShareLinkWidget::slotExpireDateSet()
{
    toggleButtonAnimation(_ui->confirmExpirationDate, _ui->expirationDateProgressIndicator, _expirationDateLinkAction, false);
}

void ShareLinkWidget::slotSetExpireDate()
{
    if (!_linkShare) {
        return;
    }

    toggleButtonAnimation(_ui->confirmExpirationDate, _ui->expirationDateProgressIndicator, _expirationDateLinkAction, true);
    _ui->errorLabel->hide();
    _linkShare->setExpireDate(_ui->calendar->date());
}

void ShareLinkWidget::slotCreatePassword()
{
    if (!_linkShare || _ui->lineEdit_password->text().isEmpty()) {
        return;
    }

    toggleButtonAnimation(_ui->confirmPassword, _ui->passwordProgressIndicator, _passwordProtectLinkAction, true);
    _ui->errorLabel->hide();
    emit createPassword(_ui->lineEdit_password->text());
}

void ShareLinkWidget::slotCreateShareLink(bool clicked)
{
    Q_UNUSED(clicked);
    slotToggleShareLinkAnimation(true);
    emit createLinkShare();
}

void ShareLinkWidget::slotPasswordSet()
{
    toggleButtonAnimation(_ui->confirmPassword, _ui->passwordProgressIndicator, _passwordProtectLinkAction, false);

    _ui->lineEdit_password->setText({});

    if (_linkShare->isPasswordSet()) {
        _ui->lineEdit_password->setEnabled(true);
        _ui->lineEdit_password->setPlaceholderText(QString::fromUtf8(passwordIsSetPlaceholder));
    } else {
        _ui->lineEdit_password->setPlaceholderText({});
    }

    emit createPasswordProcessed();
}

void ShareLinkWidget::slotPasswordSetError(const int code, const QString &message)
{
    toggleButtonAnimation(_ui->confirmPassword, _ui->passwordProgressIndicator, _passwordProtectLinkAction, false);

    slotServerError(code, message);
    togglePasswordOptions(true);
    _ui->lineEdit_password->setFocus();
    emit createPasswordProcessed();
}

void ShareLinkWidget::startAnimation(const int start, const int end)
{
    auto *animation = new QPropertyAnimation(this, "maximumHeight", this);

    animation->setDuration(500);
    animation->setStartValue(start);
    animation->setEndValue(end);

    connect(animation, &QAbstractAnimation::finished, this, &ShareLinkWidget::slotAnimationFinished);
    if (end < start) // that is to remove the widget, not to show it
        connect(animation, &QAbstractAnimation::finished, this, &ShareLinkWidget::slotDeleteAnimationFinished);
    connect(animation, &QVariantAnimation::valueChanged, this, &ShareLinkWidget::resizeRequested);

    animation->start();
}

void ShareLinkWidget::slotDeleteShareFetched()
{
    slotToggleShareLinkAnimation(false);

    _linkShare.clear();
    togglePasswordOptions(false);
    toggleNoteOptions(false);
    toggleExpireDateOptions(false);
    emit deleteLinkShare();
}

void ShareLinkWidget::toggleNoteOptions(const bool enable)
{
    _ui->noteLabel->setVisible(enable);
    _ui->textEdit_note->setVisible(enable);
    _ui->confirmNote->setVisible(enable);
    _ui->textEdit_note->setText(enable && _linkShare ? _linkShare->getNote() : QString());
    
    if (!enable && _linkShare && !_linkShare->getNote().isEmpty()) {
        _linkShare->setNote({});
    } 
}

void ShareLinkWidget::slotAnimationFinished()
{
    emit resizeRequested();
    deleteLater();
}

void ShareLinkWidget::slotCreateLabel()
{
    const auto labelText = _shareLinkEdit->text();
    if (!_linkShare || _linkShare->getLabel() == labelText || labelText.isEmpty()) {
        return;
    }
    _shareLinkWidgetAction->setChecked(true);
    toggleButtonAnimation(_shareLinkButton, _shareLinkProgressIndicator, _shareLinkWidgetAction, true);
    _ui->errorLabel->hide();
    _linkShare->setLabel(_shareLinkEdit->text());
}

void ShareLinkWidget::slotLabelSet()
{
    _shareLinkWidgetAction->setChecked(false);
    toggleButtonAnimation(_shareLinkButton, _shareLinkProgressIndicator, _shareLinkWidgetAction, false);
    displayShareLinkLabel();
}

void ShareLinkWidget::slotDeleteAnimationFinished()
{
    // There is a painting bug where a small line of this widget isn't
    // properly cleared. This explicit repaint() call makes sure any trace of
    // the share widget is removed once it's destroyed. #4189
    connect(this, SIGNAL(destroyed(QObject *)), parentWidget(), SLOT(repaint()));
}

void ShareLinkWidget::slotCreateShareRequiresPassword(const QString &message)
{
    slotToggleShareLinkAnimation(message.isEmpty());

    if (!message.isEmpty()) {
        _ui->errorLabel->setText(message);
        _ui->errorLabel->show();
    }

    _passwordRequired = true;

    togglePasswordOptions(true);
}

void ShareLinkWidget::togglePasswordOptions(const bool enable)
{
    _ui->passwordLabel->setVisible(enable);
    _ui->lineEdit_password->setVisible(enable);
    _ui->confirmPassword->setVisible(enable);
    _ui->lineEdit_password->setFocus();
    
    if (!enable && _linkShare && _linkShare->isPasswordSet()) {
        _linkShare->setPassword({});
    }
}

void ShareLinkWidget::toggleExpireDateOptions(const bool enable)
{
    _ui->expirationLabel->setVisible(enable);
    _ui->calendar->setVisible(enable);
    _ui->confirmExpirationDate->setVisible(enable);
    
    const auto date = enable ? _linkShare->getExpireDate() : QDate::currentDate().addDays(1);
    _ui->calendar->setDate(date);
    _ui->calendar->setMinimumDate(date);
    _ui->calendar->setFocus();
    
    if (!enable && _linkShare && _linkShare->getExpireDate().isValid()) {
        _linkShare->setExpireDate({});
    }
}

void ShareLinkWidget::confirmAndDeleteShare()
{
    auto messageBox = new QMessageBox(
        QMessageBox::Question,
        tr("Confirm Link Share Deletion"),
        tr("<p>Do you really want to delete the public link share <i>%1</i>?</p>"
           "<p>Note: This action cannot be undone.</p>")
            .arg(shareName()),
        QMessageBox::NoButton,
        this);
    QPushButton *yesButton =
        messageBox->addButton(tr("Delete"), QMessageBox::YesRole);
    messageBox->addButton(tr("Cancel"), QMessageBox::NoRole);

    connect(messageBox, &QMessageBox::finished, this,
        [messageBox, yesButton, this]() {
            if (messageBox->clickedButton() == yesButton) {
                this->slotToggleShareLinkAnimation(true);
                this->_linkShare->deleteShare();
            }
        });
    messageBox->open();
}

QString ShareLinkWidget::shareName() const
{
    QString name = _linkShare->getName();
    if (!name.isEmpty())
        return name;
    if (!_namesSupported)
        return tr("Public link");
    return _linkShare->getToken();
}

void ShareLinkWidget::slotContextMenuButtonClicked()
{
    _linkContextMenu->exec(QCursor::pos());
}

void ShareLinkWidget::slotLinkContextMenuActionTriggered(QAction *action)
{
    const auto state = action->isChecked();
    SharePermissions perm = SharePermissionRead;

    if (action == _addAnotherLinkAction) {
        emit createLinkShare();

    } else if (action == _readOnlyLinkAction && state) {
        _linkShare->setPermissions(perm);

    } else if (action == _allowEditingLinkAction && state) {
        perm |= SharePermissionUpdate;
        _linkShare->setPermissions(perm);

    } else if (action == _allowUploadEditingLinkAction && state) {
        perm |= SharePermissionCreate | SharePermissionUpdate | SharePermissionDelete;
        _linkShare->setPermissions(perm);

    } else if (action == _allowUploadLinkAction && state) {
        perm = SharePermissionCreate;
        _linkShare->setPermissions(perm);

    } else if (action == _passwordProtectLinkAction) {
        togglePasswordOptions(state);

    } else if (action == _expirationDateLinkAction) {
        toggleExpireDateOptions(state);

    } else if (action == _noteLinkAction) {
        toggleNoteOptions(state);

    } else if (action == _unshareLinkAction) {
        confirmAndDeleteShare();
    }
}

void ShareLinkWidget::slotServerError(const int code, const QString &message)
{
    slotToggleShareLinkAnimation(false);

    qCWarning(lcSharing) << "Error from server" << code << message;
    displayError(message);
}

void ShareLinkWidget::displayError(const QString &errMsg)
{
    _ui->errorLabel->setText(errMsg);
    _ui->errorLabel->show();
}

void ShareLinkWidget::slotStyleChanged()
{
    customizeStyle();
}

void ShareLinkWidget::customizeStyle()
{
    _unshareLinkAction->setIcon(Theme::createColorAwareIcon(":/client/theme/delete.svg"));

    _addAnotherLinkAction->setIcon(Theme::createColorAwareIcon(":/client/theme/add.svg"));

    _ui->enableShareLink->setIcon(Theme::createColorAwareIcon(":/client/theme/copy.svg"));

    _ui->shareLinkIconLabel->setPixmap(Theme::createColorAwarePixmap(":/client/theme/public.svg"));

    _ui->shareLinkToolButton->setIcon(Theme::createColorAwareIcon(":/client/theme/more.svg"));

    _ui->confirmNote->setIcon(Theme::createColorAwareIcon(":/client/theme/confirm.svg"));
    _ui->confirmPassword->setIcon(Theme::createColorAwareIcon(":/client/theme/confirm.svg"));
    _ui->confirmExpirationDate->setIcon(Theme::createColorAwareIcon(":/client/theme/confirm.svg"));

    _ui->passwordProgressIndicator->setColor(QGuiApplication::palette().color(QPalette::Text));
}

void ShareLinkWidget::displayShareLinkLabel()
{
    _shareLinkElidedLabel->clear();
    if (!_linkShare->getLabel().isEmpty()) {
        _shareLinkElidedLabel->setText(QString("(%1)").arg(_linkShare->getLabel()));
    } 
}

}
