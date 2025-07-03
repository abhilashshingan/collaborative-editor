// FILE: src/client/document_editor.h
// Description: Client-side document editor with undo/redo UI support

#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QToolBar>
#include <QAction>
#include <QUndoView>
#include <memory>
#include <string>
#include "common/document/document_controller.h"

namespace collab {
namespace client {

/**
 * Document editor widget with undo/redo UI
 */
class DocumentEditor : public QWidget {
    Q_OBJECT
    
public:
    explicit DocumentEditor(QWidget* parent = nullptr);
    virtual ~DocumentEditor();
    
    /**
     * Set the document controller
     * 
     * @param controller DocumentController instance
     * @param userId ID of the local user
     */
    void setDocumentController(std::shared_ptr<DocumentController> controller, const std::string& userId);
    
    /**
     * Get the current document text
     * 
     * @return Current document text
     */
    QString getDocument() const;
    
    /**
     * Check if document can be undone
     * 
     * @return true if undo is available
     */
    bool canUndo() const;
    
    /**
     * Check if document can be redone
     * 
     * @return true if redo is available
     */
    bool canRedo() const;

public slots:
    /**
     * Undo the last local operation
     */
    void undo();
    
    /**
     * Redo the last undone local operation
     */
    void redo();
    
private:
    QTextEdit* textEdit_;
    QToolBar* toolbar_;
    QAction* undoAction_;
    QAction* redoAction_;
    
    std::shared_ptr<DocumentController> controller_;
    std::string userId_;
    QString lastKnownText_;
    
    // Flag to ignore text changes during programmatic updates
    bool ignoreTextChanges_;
    
    void setupUi();
    void setupConnections();
    
    // Create operation from text edit changes
    ot::OperationPtr createOperationFromChange(const QString& oldText, const QString& newText);
    
    // Update editor state based on undo/redo availability
    void updateUndoRedoActions();

private slots:
    void onTextChanged();
    void onDocumentChanged(const std::string& document, int64_t revision);
};

} // namespace client
} // namespace collab