/*
  This file is part of Ingen.
  Copyright 2018 David Robillard <http://drobilla.net/>

  Ingen is free software: you can redistribute it and/or modify it under the
  terms of the GNU Affero General Public License as published by the Free
  Software Foundation, either version 3 of the License, or any later version.

  Ingen is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
  A PARTICULAR PURPOSE.  See the GNU Affero General Public License for details.

  You should have received a copy of the GNU Affero General Public License
  along with Ingen.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <boost/utility/string_view.hpp>

#include "ingen/FilePath.hpp"
#include "test_utils.hpp"

using Ingen::FilePath;

int
main(int, char**)
{
	EXPECT_EQ(FilePath("/").parent_path(), FilePath("/"));

	EXPECT_TRUE(FilePath("/abs").is_absolute())
	EXPECT_FALSE(FilePath("/abs").is_relative())
	EXPECT_EQ(FilePath("/abs").root_name(), FilePath());
	EXPECT_EQ(FilePath("/abs").root_directory(), FilePath("/"));
	EXPECT_EQ(FilePath("/abs").root_path(), FilePath("/"));
	EXPECT_EQ(FilePath("/abs").relative_path(), FilePath("abs"));
	EXPECT_EQ(FilePath("/abs").parent_path(), FilePath("/"));
	EXPECT_EQ(FilePath("/abs").filename(), FilePath("abs"));
	EXPECT_EQ(FilePath("/abs").stem(), FilePath("abs"));
	EXPECT_EQ(FilePath("/abs").extension(), FilePath());

	EXPECT_FALSE(FilePath("rel").is_absolute())
	EXPECT_TRUE(FilePath("rel").is_relative())
	EXPECT_EQ(FilePath("rel").root_name(), FilePath());
	EXPECT_EQ(FilePath("rel").root_directory(), FilePath());
	EXPECT_EQ(FilePath("rel").root_path(), FilePath());
	EXPECT_EQ(FilePath("rel").relative_path(), FilePath());
	EXPECT_EQ(FilePath("rel").parent_path(), FilePath());
	EXPECT_EQ(FilePath("rel").filename(), "rel");
	EXPECT_EQ(FilePath("rel").stem(), "rel");
	EXPECT_EQ(FilePath("rel").extension(), FilePath());

	EXPECT_FALSE(FilePath("file.txt").is_absolute())
	EXPECT_TRUE(FilePath("file.txt").is_relative())
	EXPECT_EQ(FilePath("file.txt").filename(), "file.txt");
	EXPECT_EQ(FilePath("file.txt").stem(), "file");
	EXPECT_EQ(FilePath("file.txt").extension(), ".txt");

	EXPECT_TRUE(FilePath("/abs/file.txt").is_absolute())
	EXPECT_FALSE(FilePath("/abs/file.txt").is_relative())
	EXPECT_EQ(FilePath("/abs/file.txt").filename(), "file.txt");
	EXPECT_EQ(FilePath("/abs/file.txt").stem(), "file");
	EXPECT_EQ(FilePath("/abs/file.txt").extension(), ".txt");

	EXPECT_FALSE(FilePath("rel/file.txt").is_absolute())
	EXPECT_TRUE(FilePath("rel/file.txt").is_relative())
	EXPECT_EQ(FilePath("rel/file.txt").filename(), "file.txt");
	EXPECT_EQ(FilePath("rel/file.txt").stem(), "file");
	EXPECT_EQ(FilePath("rel/file.txt").extension(), ".txt");

	FilePath path("/x");
	EXPECT_EQ(path, "/x");
	path = std::string("/a");
	EXPECT_EQ(path, "/a");

	path /= FilePath("b");
	EXPECT_EQ(path, "/a/b");

	path += FilePath("ar");
	EXPECT_EQ(path, "/a/bar");

	path += std::string("/c");
	EXPECT_EQ(path, "/a/bar/c");

	path += "a";
	EXPECT_EQ(path, "/a/bar/ca");

	path += 'r';
	EXPECT_EQ(path, "/a/bar/car");

	path += boost::string_view("/d");
	EXPECT_EQ(path, "/a/bar/car/d");

	const FilePath apple("apple");
	const FilePath zebra("zebra");
	EXPECT_TRUE(apple == apple);
	EXPECT_TRUE(apple != zebra);
	EXPECT_TRUE(apple < zebra);
	EXPECT_TRUE(apple <= zebra);
	EXPECT_TRUE(apple <= apple);
	EXPECT_TRUE(zebra > apple);
	EXPECT_TRUE(zebra >= apple);
	EXPECT_TRUE(zebra >= zebra);
	return 0;
}
