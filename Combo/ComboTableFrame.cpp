/// \file
/// \author Xavier Michelon
///
/// \brief Declaration of combo table frame
///  
/// Copyright (c) Xavier Michelon. All rights reserved.  
/// Licensed under the MIT License. See LICENSE file in the project root for full license information.  


#include "stdafx.h"
#include "ComboTableFrame.h"
#include "ComboManager.h"
#include "ComboDialog.h"


//**********************************************************************************************************************
/// brief A class overriding the default style only to remove the focus rectangle around items in a table view
//**********************************************************************************************************************
class ComboTableProxyStyle : public QProxyStyle
{
public:
   virtual void drawPrimitive(PrimitiveElement element, const QStyleOption * option,
      QPainter * painter, const QWidget * widget = 0) const
   {
      if (PE_FrameFocusRect != element)
         QProxyStyle::drawPrimitive(element, option, painter, widget);
   }
};



//**********************************************************************************************************************
/// \param[in] parent The parent widget of the frame
//**********************************************************************************************************************
ComboTableFrame::ComboTableFrame(QWidget* parent)
   : QFrame(parent)
   , proxyStyle_(std::make_unique<ComboTableProxyStyle>())
{
   ui_.setupUi(this);
   this->setupTable();
   this->setupContextMenu();
   this->updateGui();
   connect(new QShortcut(QKeySequence("Ctrl+F"), this), &QShortcut::activated,
      this, [&]() { this->ui_.editSearch->setFocus(); this->ui_.editSearch->selectAll(); });
   connect(new QShortcut(QKeySequence("Escape"), this), &QShortcut::activated,
      this, [&]() { this->ui_.editSearch->setFocus(); this->ui_.editSearch->clear(); });
   connect(new QShortcut(QKeySequence("Ctrl+N"), this), &QShortcut::activated,
      this, &ComboTableFrame::onActionAddCombo);
   connect(new QShortcut(QKeySequence("Delete"), this), &QShortcut::activated, 
      this, &ComboTableFrame::onActionDeleteCombo);
   connect(new QShortcut(QKeySequence("Ctrl+Shift+N"), this), &QShortcut::activated,
      this, &ComboTableFrame::onActionDuplicateCombo);
   connect(new QShortcut(QKeySequence(Qt::Key_Return), this), &QShortcut::activated,
      this, &ComboTableFrame::onActionEditCombo);
   connect(new QShortcut(QKeySequence("Ctrl+A"), this), &QShortcut::activated,
      this, &ComboTableFrame::onActionSelectAll);
   connect(new QShortcut(QKeySequence("Ctrl+D"), this), &QShortcut::activated,
      this, &ComboTableFrame::onActionDeselectAll);
}



//**********************************************************************************************************************
// 
//**********************************************************************************************************************
void ComboTableFrame::setupTable()
{
   proxyModel_.setSourceModel(&ComboManager::instance().getComboListRef());
   ui_.tableComboList->setModel(&proxyModel_);
   proxyModel_.sort(0, Qt::AscendingOrder);
   QHeaderView* header = ui_.tableComboList->horizontalHeader();
   header->setSortIndicator(0, Qt::AscendingOrder);  //< required, otherwise the indicator is first displayed in the wrong direction
   header->setDefaultAlignment(Qt::AlignLeft);
   connect(ui_.tableComboList->selectionModel(), &QItemSelectionModel::selectionChanged, this,
      &ComboTableFrame::updateGui);
   connect(ui_.tableComboList, &QTableView::doubleClicked, this, &ComboTableFrame::onActionEditCombo);
   QHeaderView *verticalHeader = ui_.tableComboList->verticalHeader();
   verticalHeader->setDefaultSectionSize(verticalHeader->fontMetrics().height() + 10);
   ui_.tableComboList->setStyle(proxyStyle_.get());
}


//**********************************************************************************************************************
// 
//**********************************************************************************************************************
void ComboTableFrame::setupContextMenu()
{
   contextMenu_.clear();
   contextMenu_.addAction(ui_.actionAddCombo);
   contextMenu_.addAction(ui_.actionDuplicateCombo);
   contextMenu_.addAction(ui_.actionDeleteCombo);
   contextMenu_.addAction(ui_.actionEditCombo);
   contextMenu_.addSeparator();
   contextMenu_.addAction(ui_.actionSelectAll);
   contextMenu_.addAction(ui_.actionDeselectAll);
   ui_.tableComboList->setContextMenuPolicy(Qt::CustomContextMenu);
   connect(ui_.tableComboList, &QTableView::customContextMenuRequested, this, &ComboTableFrame::onContextMenuRequested);
}

//**********************************************************************************************************************
/// \return The number of selected combos
//**********************************************************************************************************************
qint32 ComboTableFrame::getSelectedComboCount() const
{
   return ui_.tableComboList->selectionModel()->selectedRows().size();
}


//**********************************************************************************************************************
/// The returned indexes are based on the internal order of the list, not the display order in the table (that can be
/// modified by sorting by columns).
///
/// \return The list of indexes of the selected combos 
//**********************************************************************************************************************
QList<qint32> ComboTableFrame::getSelectedComboIndexes() const
{
   QList<qint32> result;
   ComboList& comboList = ComboManager::instance().getComboListRef();
   QModelIndexList selectedRows = ui_.tableComboList->selectionModel()->selectedRows();
   for (QModelIndex const& modelIndex : selectedRows)
   {
      qint32 const index = proxyModel_.mapToSource(modelIndex).row();
      if ((index >= 0) && (index < comboList.size()))
         result.push_back(index);
   }
   return result;
}

//**********************************************************************************************************************
// 
//**********************************************************************************************************************
void ComboTableFrame::updateGui()
{
   qint32 const selectedCount = this->getSelectedComboCount();
   bool const hasOneSelected = (1 == selectedCount);
   bool const hasOneOrMoreSelected = (selectedCount > 0);
   ui_.buttonDuplicateCombo->setEnabled(hasOneSelected);
   ui_.actionDuplicateCombo->setEnabled(hasOneSelected);
   ui_.buttonDeleteCombo->setEnabled(hasOneOrMoreSelected);
   ui_.actionDeleteCombo->setEnabled(hasOneOrMoreSelected);
   ui_.buttonEditCombo->setEnabled(hasOneSelected);
   ui_.actionEditCombo->setEnabled(hasOneSelected);

   bool const hasItem = !(ComboManager::instance().getComboListRef().rowCount() > 0);
}


//**********************************************************************************************************************
// 
//**********************************************************************************************************************
void ComboTableFrame::onActionAddCombo()
{
   SPCombo combo = Combo::create();
   ComboDialog dlg(combo);
   if (QDialog::Accepted != dlg.exec())
      return;
   ComboManager& comboManager = ComboManager::instance();
   ComboList& comboList = ComboManager::instance().getComboListRef();
   comboList.append(combo);
   QString errorMessage;
   if (!comboManager.saveComboListToFile(&errorMessage))
      QMessageBox::critical(this, tr("Error"), errorMessage);
}


//**********************************************************************************************************************
// 
//**********************************************************************************************************************
void ComboTableFrame::onActionDuplicateCombo()
{
   QList<qint32> const selectedIndex = this->getSelectedComboIndexes();
   if (1 != selectedIndex.size())
      return;
   ComboManager& comboManager = ComboManager::instance();
   ComboList& comboList = comboManager.getComboListRef();
   qint32 index = selectedIndex[0];
   Q_ASSERT((index >= 0) && (index < comboList.size()));
   SPCombo combo = Combo::duplicate(*comboList[index]);

   ComboDialog dlg(combo);
   if (QDialog::Accepted != dlg.exec())
      return;
   comboList.append(combo);
}

//**********************************************************************************************************************
// 
//**********************************************************************************************************************
void ComboTableFrame::onActionDeleteCombo()
{
   qint32 const count = this->getSelectedComboCount();
   if (count < 1)
      return;
   QString question = count > 1 ? tr("Are you sure you want to delete the selected combos?")
      : tr("Are you sure you want to delete the selected combo?");
   if (QMessageBox::Yes != QMessageBox::question(nullptr, tr("Delete Combo?"), question,
      QMessageBox::Yes | QMessageBox::No, QMessageBox::No))
      return;

   ComboManager& comboManager = ComboManager::instance();
   QList<qint32> indexes = this->getSelectedComboIndexes();
   std::sort(indexes.begin(), indexes.end(), [](qint32 first, qint32 second) -> bool { return first > second; });
   for (qint32 index : indexes)
      comboManager.getComboListRef().erase(index);
   QString errorMessage;
   if (!comboManager.saveComboListToFile(&errorMessage))
      QMessageBox::critical(this, tr("Error"), errorMessage);
}


//**********************************************************************************************************************
// 
//**********************************************************************************************************************
void ComboTableFrame::onActionEditCombo()
{
   QList<qint32> const selectedIndex = this->getSelectedComboIndexes();
   if (1 != selectedIndex.size())
      return;

   ComboManager& comboManager = ComboManager::instance();
   ComboList& comboList = comboManager.getComboListRef();
   qint32 index = selectedIndex[0];
   Q_ASSERT((index >= 0) && (index < comboList.size()));
   SPCombo combo = comboList[index];
   ComboDialog dlg(combo);
   if (QDialog::Accepted != dlg.exec())
      return;
   comboList.markComboAsEdited(index);
   QString errorMessage;
   if (!comboManager.saveComboListToFile(&errorMessage))
      QMessageBox::critical(this, tr("Error"), errorMessage);
}


//**********************************************************************************************************************
// 
//**********************************************************************************************************************
void ComboTableFrame::onActionSelectAll()
{
   ui_.tableComboList->selectAll();
}


//**********************************************************************************************************************
// 
//**********************************************************************************************************************
void ComboTableFrame::onActionDeselectAll()
{
   ui_.tableComboList->clearSelection();
}

//**********************************************************************************************************************
/// \param[in] text The text to search
//**********************************************************************************************************************
void ComboTableFrame::onSearchFilterChanged(QString const& text)
{
   QString const searchStr = text.trimmed();
   proxyModel_.setFilterFixedString(searchStr);

}

//**********************************************************************************************************************
// 
//**********************************************************************************************************************
void ComboTableFrame::onContextMenuRequested()
{
   contextMenu_.exec(QCursor::pos());
}
