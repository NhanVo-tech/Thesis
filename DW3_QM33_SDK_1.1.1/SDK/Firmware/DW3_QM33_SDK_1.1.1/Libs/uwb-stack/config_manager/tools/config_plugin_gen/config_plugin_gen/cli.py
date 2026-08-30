"""CLI module."""

import sys
import os
import traceback
import click
from loguru import logger
from config_plugin_gen import models
from config_plugin_gen import Generator
from config_plugin_gen.models import ConfigModel
import pathlib
from pydantic_yaml import parse_yaml_file_as
import json
import yaml


@click.command()
@click.version_option()
@click.argument("input_file", type=click.Path(exists=True, readable=True))
@click.argument("output_dir", type=click.Path(dir_okay=True))
@click.option("-D", "--define", multiple=True)
def cli_root(input_file: str, output_dir: str, define: list[str]) -> None:
    """Declare the generate config_manager command."""
    try:
        with models.init_context({"defines": define}):
            model = parse_yaml_file_as(ConfigModel, input_file)

        converted_model = model.convert_to_model()

        generator = Generator()
        # Build list of defines
        generator.generate(model, pathlib.Path(output_dir))
    except Exception as e:
        logger.error(
            f"Unable to parse the YAML model \
            '{os.path.basename(input_file)} {e}'"
        )
        logger.debug(traceback.format_exc())
        sys.exit(-1)


if __name__ == "__main__":
    sys.exit(cli_root())
