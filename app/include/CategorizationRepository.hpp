#ifndef CATEGORIZATION_REPOSITORY_HPP
#define CATEGORIZATION_REPOSITORY_HPP

#include "Types.hpp"
#include "Result.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace afs {

/**
 * @brief Criteria for querying categorizations.
 */
struct CategorizationQuery {
    std::string directory_path;
    std::optional<FileType> file_type;
    std::optional<std::string> category;
    std::optional<bool> used_consistency_hints;
    std::optional<bool> from_cache;
    size_t limit{0};  // 0 = no limit
    size_t offset{0};
};

/**
 * @brief Statistics about the categorization repository.
 */
struct RepositoryStats {
    int64_t total_entries{0};
    int64_t file_entries{0};
    int64_t directory_entries{0};
    int64_t taxonomy_entries{0};
    int64_t database_size_bytes{0};
    std::string oldest_entry_date;
    std::string newest_entry_date;
};

/**
 * @brief Repository pattern interface for categorization data access.
 * 
 * Provides a clean abstraction over the database layer for:
 * - File/directory categorization storage and retrieval
 * - Taxonomy management
 * - Cache operations
 * 
 * This interface enables:
 * - Easy testing with mock implementations
 * - Potential future backend changes (different databases, cloud storage)
 * - Consistent error handling across all data operations
 */
class ICategorizationRepository {
public:
    virtual ~ICategorizationRepository() = default;

    // =====================
    // Categorization CRUD
    // =====================

    /**
     * @brief Saves or updates a categorization.
     * @param entry The categorized file/directory to save
     * @return Result indicating success or error
     */
    virtual Result<void> save(const CategorizedFile& entry) = 0;

    /**
     * @brief Retrieves a categorization by path and name.
     * @param directory_path The directory containing the file
     * @param file_name The name of the file/directory
     * @param type Whether it's a file or directory
     * @return Optional containing the entry if found
     */
    virtual Result<std::optional<CategorizedFile>> find_one(
        const std::string& directory_path,
        const std::string& file_name,
        FileType type) = 0;

    /**
     * @brief Queries categorizations matching criteria.
     * @param query The search criteria
     * @return Vector of matching entries
     */
    virtual Result<std::vector<CategorizedFile>> find(const CategorizationQuery& query) = 0;

    /**
     * @brief Gets all categorizations for a directory.
     * @param directory_path The directory path
     * @return Vector of categorized entries
     */
    virtual Result<std::vector<CategorizedFile>> find_by_directory(
        const std::string& directory_path) = 0;

    /**
     * @brief Removes a categorization.
     * @param directory_path The directory containing the file
     * @param file_name The name of the file/directory
     * @param type Whether it's a file or directory
     * @return true if an entry was removed
     */
    virtual Result<bool> remove(
        const std::string& directory_path,
        const std::string& file_name,
        FileType type) = 0;

    /**
     * @brief Removes all categorizations for a directory.
     * @param directory_path The directory path
     * @return Number of entries removed
     */
    virtual Result<int> remove_by_directory(const std::string& directory_path) = 0;

    /**
     * @brief Removes entries with empty categories.
     * @param directory_path The directory to clean up
     * @return Vector of removed entries (for potential undo)
     */
    virtual Result<std::vector<CategorizedFile>> remove_empty_categorizations(
        const std::string& directory_path) = 0;

    // =====================
    // Taxonomy Operations
    // =====================

    /**
     * @brief Resolves a category/subcategory to its canonical form.
     * Uses fuzzy matching to find existing similar entries.
     * @param category The category name
     * @param subcategory The subcategory name
     * @return Resolved category with taxonomy ID
     */
    struct ResolvedCategory {
        int taxonomy_id{0};
        std::string category;
        std::string subcategory;
    };
    virtual Result<ResolvedCategory> resolve_category(
        const std::string& category,
        const std::string& subcategory) = 0;

    /**
     * @brief Gets the most frequently used categories.
     * @param max_entries Maximum number of entries to return
     * @return Vector of category/subcategory pairs
     */
    virtual Result<std::vector<std::pair<std::string, std::string>>> get_top_categories(
        size_t max_entries) = 0;

    /**
     * @brief Gets recent categories used for files with a specific extension.
     * @param extension File extension (e.g., ".pdf")
     * @param type File type (file or directory)
     * @param limit Maximum entries to return
     * @return Vector of category/subcategory pairs
     */
    virtual Result<std::vector<std::pair<std::string, std::string>>> get_categories_for_extension(
        const std::string& extension,
        FileType type,
        size_t limit) = 0;

    // =====================
    // Metadata Operations
    // =====================

    /**
     * @brief Gets the categorization style for a directory.
     * @param directory_path The directory path
     * @return Optional bool (true=consistency hints used, false=not used, empty=unknown)
     */
    virtual Result<std::optional<bool>> get_directory_style(
        const std::string& directory_path) = 0;

    /**
     * @brief Checks if a file has been categorized.
     * @param file_name The file name
     * @return true if categorization exists
     */
    virtual Result<bool> exists(const std::string& file_name) = 0;

    // =====================
    // Maintenance Operations
    // =====================

    /**
     * @brief Gets repository statistics.
     */
    virtual Result<RepositoryStats> get_stats() = 0;

    /**
     * @brief Clears all cached data.
     * @return true if successful
     */
    virtual Result<bool> clear_all() = 0;

    /**
     * @brief Clears entries older than specified days.
     * @param days Number of days
     * @return Number of entries cleared
     */
    virtual Result<int> clear_older_than(int days) = 0;

    /**
     * @brief Optimizes the database storage.
     * @return true if successful
     */
    virtual Result<bool> optimize() = 0;

    /**
     * @brief Begins a transaction.
     * @return Result indicating success
     */
    virtual Result<void> begin_transaction() = 0;

    /**
     * @brief Commits the current transaction.
     * @return Result indicating success
     */
    virtual Result<void> commit_transaction() = 0;

    /**
     * @brief Rolls back the current transaction.
     * @return Result indicating success
     */
    virtual Result<void> rollback_transaction() = 0;
};

/**
 * @brief RAII transaction guard for automatic rollback on error.
 * 
 * Usage:
 *   {
 *       auto guard = TransactionGuard::begin(repository);
 *       if (!guard) return guard.error();
 *       
 *       // ... perform operations ...
 *       
 *       guard->commit();  // No-op if not called; destructor will rollback
 *   }
 */
class TransactionGuard {
public:
    static Result<TransactionGuard> begin(ICategorizationRepository& repo) {
        auto result = repo.begin_transaction();
        if (!result) {
            return result.error();
        }
        return TransactionGuard(repo);
    }

    ~TransactionGuard() {
        if (!committed_) {
            repo_.rollback_transaction();  // Best effort, ignore result
        }
    }

    Result<void> commit() {
        auto result = repo_.commit_transaction();
        if (result) {
            committed_ = true;
        }
        return result;
    }

    // Non-copyable, movable
    TransactionGuard(const TransactionGuard&) = delete;
    TransactionGuard& operator=(const TransactionGuard&) = delete;
    TransactionGuard(TransactionGuard&& other) noexcept 
        : repo_(other.repo_), committed_(other.committed_) {
        other.committed_ = true;  // Prevent other from rolling back
    }
    TransactionGuard& operator=(TransactionGuard&&) = delete;

private:
    explicit TransactionGuard(ICategorizationRepository& repo) : repo_(repo) {}

    ICategorizationRepository& repo_;
    bool committed_{false};
};

/**
 * @brief Factory for creating repository instances.
 */
class CategorizationRepositoryFactory {
public:
    /**
     * @brief Creates a repository with the default SQLite backend.
     * @param config_dir Directory for database storage
     * @return Repository instance or error
     */
    static Result<std::unique_ptr<ICategorizationRepository>> create_sqlite(
        const std::string& config_dir);

    /**
     * @brief Creates an in-memory repository for testing.
     * @return Repository instance
     */
    static std::unique_ptr<ICategorizationRepository> create_memory();
};

} // namespace afs

#endif // CATEGORIZATION_REPOSITORY_HPP
