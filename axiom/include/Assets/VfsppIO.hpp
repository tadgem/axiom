#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "assimp/IOStream.hpp"
#include "assimp/IOSystem.hpp"
#include "vfspp/VFS.h"

namespace axm
{

/// <summary>
/// Assimp IOStream implementation backed by a vfspp file handle.
/// vfspp owns the underlying file; this class keeps it open until Assimp closes
/// (or deletes) the stream.
/// </summary>
class VfsppIOStream final : public Assimp::IOStream
{
public:
	explicit VfsppIOStream(vfspp::IFilePtr file) : m_File(std::move(file)) {}

	~VfsppIOStream() override
	{
		if (m_File && m_File->IsOpened())
		{
			m_File->Close();
		}
	}

	size_t Read(void* pvBuffer, size_t pSize, size_t pCount) override
	{
		if (!m_File || !m_File->IsOpened() || pvBuffer == nullptr || pSize == 0 || pCount == 0)
		{
			return 0;
		}

		if (pCount > (std::numeric_limits<size_t>::max)() / pSize)
		{
			return 0;
		}

		const auto bytes  = pSize * pCount;
		const auto buffer = std::span<std::uint8_t>(static_cast<std::uint8_t*>(pvBuffer), bytes);
		const auto read   = static_cast<size_t>(m_File->Read(buffer));

		// Assimp follows fread semantics: return the number of whole items read.
		return read / pSize;
	}

	size_t Write(const void* pvBuffer, size_t pSize, size_t pCount) override
	{
		if (!m_File || !m_File->IsOpened() || pvBuffer == nullptr || pSize == 0 || pCount == 0)
		{
			return 0;
		}

		if (pCount > (std::numeric_limits<size_t>::max)() / pSize)
		{
			return 0;
		}

		const auto bytes   = pSize * pCount;
		const auto buffer  = std::span<const std::uint8_t>(static_cast<const std::uint8_t*>(pvBuffer), bytes);
		const auto written = static_cast<size_t>(m_File->Write(buffer));

		return written / pSize;
	}

	aiReturn Seek(size_t pOffset, aiOrigin pOrigin) override
	{
		if (!m_File || !m_File->IsOpened())
		{
			return AI_FAILURE;
		}

		// vfspp names SEEK_CUR "Set"; vfspp::IFile::Origin::Begin is SEEK_SET and
		// vfspp::IFile::Origin::End is SEEK_END.
		vfspp::IFile::Origin origin = vfspp::IFile::Origin::Begin;
		switch (pOrigin)
		{
			case aiOrigin_SET:
				origin = vfspp::IFile::Origin::Begin;
				break;
			case aiOrigin_CUR:
				origin = vfspp::IFile::Origin::Set;
				break;
			case aiOrigin_END:
				origin = vfspp::IFile::Origin::End;
				break;
			default:
				return AI_FAILURE;
		}

		// Assimp documents pOffset as negative for aiOrigin_CUR / aiOrigin_END
		// (it is a size_t carrying a two's-complement value). Preserve the bit
		// pattern so vfspp/fseek observes the intended signed offset.
		m_File->Seek(static_cast<std::uint64_t>(pOffset), origin);
		return AI_SUCCESS;
	}

	size_t Tell() const override { return m_File ? static_cast<size_t>(m_File->Tell()) : 0; }

	size_t FileSize() const override { return m_File ? static_cast<size_t>(m_File->Size()) : 0; }

	void   Flush() override {}

private:
	vfspp::IFilePtr m_File;
};

/// <summary>
/// Assimp IOSystem implementation that resolves every file reference through a
/// vfspp VirtualFileSystem. This is what allows Assimp to find sibling files
/// referenced by a model (.bin buffers, textures, .mtl files, ...) instead of
/// looking on the host filesystem.
/// </summary>
class VfsppIOSystem final : public Assimp::IOSystem
{
public:
	explicit VfsppIOSystem(vfspp::VirtualFileSystem& vfs) : m_VFS(vfs) {}

	bool Exists(const char* pFile) const override
	{
		const std::string path = ToVirtualPath(pFile);
		return !path.empty() && m_VFS.IsFileExists(path);
	}

	char getOsSeparator() const override
	{
		// vfspp virtual paths always use forward slashes.
		return '/';
	}

	Assimp::IOStream* Open(const char* pFile, const char* pMode) override
	{
		const std::string path = ToVirtualPath(pFile);
		if (path.empty())
		{
			return nullptr;
		}

		auto file = m_VFS.OpenFile(path, ParseFileMode(pMode));
		if (!file || !file->IsOpened())
		{
			return nullptr;
		}

		// The IOSystem owns every stream it hands to Assimp; Importer calls
		// Close() (or deletes the stream) when it is done with it.
		return new VfsppIOStream(std::move(file));
	}

	void Close(Assimp::IOStream* pFile) override { delete pFile; }

	bool ComparePaths(const char* one, const char* second) const override
	{
		if (one == nullptr || second == nullptr)
		{
			return one == second;
		}

		return ToLowerCopy(ToVirtualPath(one)) == ToLowerCopy(ToVirtualPath(second));
	}

private:
	/// <summary>
	/// Assimp hands these shims paths exactly as they are stored in the model
	/// file (or as supplied by the caller). vfspp matches virtual paths against
	/// a registered alias, and axiom registers its filesystems under "/", so
	/// separators must be normalised and the path must be rooted at the virtual
	/// filesystem root.
	/// </summary>
	static std::string ToVirtualPath(const char* pFile)
	{
		if (pFile == nullptr)
		{
			return {};
		}

		std::string path(pFile);
		std::replace(path.begin(), path.end(), '\\', '/');

		// Collapse repeated separators; Assimp's FileSystemFilter can produce
		// them when it appends a base directory to a relative reference.
		std::string normalized;
		normalized.reserve(path.size() + 1);
		for (const char c : path)
		{
			if (c == '/' && !normalized.empty() && normalized.back() == '/')
			{
				continue;
			}
			normalized.push_back(c);
		}

		if (normalized.empty())
		{
			return {};
		}

		if (normalized.front() != '/')
		{
			normalized.insert(normalized.begin(), '/');
		}

		return normalized;
	}

	static std::string ToLowerCopy(std::string_view value)
	{
		std::string result(value);
		for (auto& c : result)
		{
			if (c >= 'A' && c <= 'Z')
			{
				c = static_cast<char>(c - 'A' + 'a');
			}
		}
		return result;
	}

	static vfspp::IFile::FileMode ParseFileMode(const char* pMode)
	{
		// Assimp requires at least "rb", "wb" or "ab" style modes.
		auto mode = vfspp::IFile::FileMode::Read;
		if (pMode == nullptr)
		{
			return mode;
		}

		const std::string_view modeView(pMode);
		if (!modeView.empty())
		{
			switch (modeView.front())
			{
				case 'w':
					mode = vfspp::IFile::FileMode::Write | vfspp::IFile::FileMode::Truncate;
					break;
				case 'a':
					mode = vfspp::IFile::FileMode::Write | vfspp::IFile::FileMode::Append;
					break;
				default:
					mode = vfspp::IFile::FileMode::Read;
					break;
			}
		}

		// "r+", "w+" and "a+" also request read access.
		if (modeView.find('+') != std::string_view::npos)
		{
			mode = mode | vfspp::IFile::FileMode::Read;
		}

		return mode;
	}

private:
	vfspp::VirtualFileSystem& m_VFS;
};

}  // namespace axm
